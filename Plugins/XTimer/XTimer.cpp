/*

  XTimer.cpp
  CrossBasic Plugin: XTimer                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
  Copyright (c) 2025 Simulanics Technologies – Matthew Combatti
  All rights reserved.
 
  Licensed under the CrossBasic Source License (CBSL-1.1).
  You may not use this file except in compliance with the License.
  You may obtain a copy of the License at:
  https://www.crossbasic.com/license
 
  SPDX-License-Identifier: CBSL-1.1
  
  Author:
    The AI Team under direction of Matthew Combatti <mcombatti@crossbasic.com>
    
*/

/*  ── Build commands ────────────────────────────────────────────────────────────
  Windows (MinGW-w64):
    g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ ^
        -o XTimer.dll XTimer.cpp -pthread

  Linux:
    g++ -std=c++17 -shared -fPIC -o libXTimer.so XTimer.cpp -pthread

  macOS:
    g++ -std=c++17 -dynamiclib -o libXTimer.dylib XTimer.cpp -pthread
*/

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <string>
#include <cstdlib>     // strdup
#include <thread>
#include <condition_variable>
#include <chrono>
#include <random>
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

//==============================================================================
//  Debug
//==============================================================================
#define DBG_PREFIX "XTimer DEBUG: "
#define DBG(msg)  do { std::cout << DBG_PREFIX << msg << std::endl; } while (0)

// forward triggerEvent
static void triggerEvent(int handle, const std::string& eventName, const char* param);

//==============================================================================
//  XTimer instance
//==============================================================================
class XTimer {
public:
    int handle;
    std::atomic<bool> enabled{false};
    std::atomic<int> period{1000};
    std::atomic<int> runMode{0};
    std::atomic<bool> exiting{false};

    std::mutex mtx;
    std::condition_variable cv;
    std::thread thr;

    explicit XTimer(int h) : handle(h) {
        //DBG("Constructor(): handle=" << handle);
        thr = std::thread([this]() {
            std::unique_lock<std::mutex> lk(mtx);
            while (!exiting) {
                cv.wait(lk, [this]{ return enabled || exiting; });
                if (exiting) break;
                if (runMode == 1) {
                    // single-shot
                    auto ms = std::chrono::milliseconds(period.load());
                    if (!cv.wait_for(lk, ms, [this]{ return !enabled || exiting; })) {
                        ::triggerEvent(handle, "Action", nullptr);
                        enabled = false;
                    }
                }
                else if (runMode == 2) {
                    // periodic
                    while (enabled && !exiting) {
                        auto ms = std::chrono::milliseconds(period.load());
                        if (cv.wait_for(lk, ms, [this]{ return !enabled || exiting; })) {
                            break;
                        }
                        ::triggerEvent(handle, "Action", nullptr);
                    }
                }
                // if runMode==0, just go back to waiting
            }
            //DBG("Thread exiting for handle=" << handle);
        });
    }

    ~XTimer() {
        exiting = true;
        cv.notify_all();
        if (thr.joinable()) thr.join();
        //DBG("Destructor(): handle=" << handle);
    }
};

//==============================================================================
//  Globals
//==============================================================================
static std::mutex g_timersMtx;
static std::unordered_map<int, XTimer*> g_timers;

static std::mutex g_eventMtx;
static std::unordered_map<int, std::unordered_map<std::string, void*>> g_timerEvents;

//==============================================================================
//  triggerEvent implementation
//==============================================================================
static void triggerEvent(int handle, const std::string& eventName, const char* param) {
    void* cb = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        auto it = g_timerEvents.find(handle);
        if (it != g_timerEvents.end()) {
            auto jt = it->second.find(eventName);
            if (jt != it->second.end()) cb = jt->second;
        }
    }
    if (!cb) return;
    using CB = void(*)(const char*);
    char* data = strdup(param ? param : "");
    //DBG("Invoking Action for handle=" << handle);
    ((CB)cb)(data);
    free(data);
}

extern "C" {

//------------------------------------------------------------------------------
// Constructor / Destructor
//------------------------------------------------------------------------------
XPLUGIN_API int Constructor() {
    int h;
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<int> dist(10000000, 99999999);
    {
        std::lock_guard<std::mutex> lk(g_timersMtx);
        do { h = dist(rng); } while (g_timers.count(h));
        g_timers[h] = new XTimer(h);
    }
    return h;
}

XPLUGIN_API void Close(int handle) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    auto it = g_timers.find(handle);
    if (it != g_timers.end()) {
        delete it->second;
        g_timers.erase(it);
        std::lock_guard<std::mutex> lk2(g_eventMtx);
        g_timerEvents.erase(handle);
        //DBG("Closed handle=" << handle);
    }
}

//------------------------------------------------------------------------------
// Event-property getter: OnAction
//------------------------------------------------------------------------------
XPLUGIN_API const char* Action_GET(int handle) {
    std::string s = "XTimer:" + std::to_string(handle) + ":Action";
    return strdup(s.c_str());
}

//------------------------------------------------------------------------------
// Register callback
//------------------------------------------------------------------------------
XPLUGIN_API bool XTimer_SetEventCallback(int handle,
                                         const char* eventName,
                                         void* callback) {
    {
        std::lock_guard<std::mutex> lk(g_timersMtx);
        if (!g_timers.count(handle)) return false;
    }
    std::string key = eventName ? eventName : "";
    auto pos = key.rfind(':');
    if (pos != std::string::npos) key.erase(0, pos + 1);
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        g_timerEvents[handle][key] = callback;
    }
    return true;
}

//------------------------------------------------------------------------------
// Enabled (boolean)
//------------------------------------------------------------------------------
XPLUGIN_API void XTimer_Enabled_SET(int handle, bool v) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    if (auto it = g_timers.find(handle); it != g_timers.end()) {
        it->second->enabled = v;
        it->second->cv.notify_all();
    }
}
XPLUGIN_API bool XTimer_Enabled_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    if (auto it = g_timers.find(handle); it != g_timers.end())
        return it->second->enabled.load();
    return false;
}

//------------------------------------------------------------------------------
// Period (integer, milliseconds)
//------------------------------------------------------------------------------
XPLUGIN_API void XTimer_Period_SET(int handle, int v) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    if (auto it = g_timers.find(handle); it != g_timers.end()) {
        it->second->period = v;
        it->second->cv.notify_all();
    }
}
XPLUGIN_API int XTimer_Period_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    if (auto it = g_timers.find(handle); it != g_timers.end())
        return it->second->period.load();
    return 0;
}

//------------------------------------------------------------------------------
// RunMode (0=Off,1=Single,2=Multiple)
//------------------------------------------------------------------------------
XPLUGIN_API void XTimer_RunMode_SET(int handle, int v) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    if (auto it = g_timers.find(handle); it != g_timers.end()) {
        it->second->runMode = v;
        it->second->cv.notify_all();
    }
}
XPLUGIN_API int XTimer_RunMode_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_timersMtx);
    if (auto it = g_timers.find(handle); it != g_timers.end())
        return it->second->runMode.load();
    return 0;
}

//------------------------------------------------------------------------------
// ClassDefinition boilerplate
//------------------------------------------------------------------------------
typedef struct { const char* name; const char* type; void* getter; void* setter; } ClassProperty;
typedef struct { const char* name; void* funcPtr; int arity; const char* paramTypes[10]; const char* retType; } ClassEntry;
typedef struct { const char* declaration; } ClassConstant;
typedef struct {
    const char*     className;
    size_t          classSize;
    void*           constructor;
    ClassProperty*  properties;
    size_t          propertiesCount;
    ClassEntry*     methods;
    size_t          methodsCount;
    ClassConstant*  constants;
    size_t          constantsCount;
} ClassDefinition;

static ClassProperty props[] = {
    { "Enabled",  "boolean", (void*)XTimer_Enabled_GET,    (void*)XTimer_Enabled_SET  },
    { "Period",   "integer", (void*)XTimer_Period_GET,     (void*)XTimer_Period_SET   },
    { "RunMode",  "integer", (void*)XTimer_RunMode_GET,    (void*)XTimer_RunMode_SET  },
    { "Action", "string",  (void*)Action_GET,          nullptr                   }
};

static ClassEntry methods[] = {
    { "XTimer_SetEventCallback", (void*)XTimer_SetEventCallback, 3,
      { "integer","string","pointer" }, "boolean" }
};

static ClassDefinition classDef = {
    "XTimer",
    sizeof(XTimer),
    (void*)Constructor,
    props,   sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &classDef;
}

} // extern "C"
