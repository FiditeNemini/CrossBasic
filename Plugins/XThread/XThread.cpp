/*

  XThread.cpp
  CrossBasic Plugin: XThread                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
      -o XThread.dll XThread.cpp -pthread

  Linux:
    g++ -std=c++17 -shared -fPIC -o libXThread.so XThread.cpp -pthread

  macOS:
    g++ -std=c++17 -dynamiclib -o libXThread.dylib XThread.cpp -pthread
*/

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <string>
#include <cstdlib>     // strdup
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <cstring> // for strdup on POSIX


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
#define DBG_PREFIX "XThread DEBUG: "
#define DBG(msg)  do { std::cout << DBG_PREFIX << msg << std::endl; } while (0)

// forward declaration
static void triggerEvent(int handle, const std::string& eventName, const char* param);

//==============================================================================
//  XThread instance
//==============================================================================
class XThread {
public:
    int handle;
    std::string tag;
    std::atomic<int> threadState{4};  // 0=Running,1=Waiting,2=Paused,3=Sleeping,4=NotRunning
    std::atomic<int> threadType{0};   // 0=Cooperative,1=Preemptive
    std::thread thr;

    explicit XThread(int h) : handle(h) {
        DBG("Constructor handle=" << h);
    }

    ~XThread() {
        // ensure the thread is joined
        if (thr.joinable()) {
            thr.join();
            threadState = 4;
        }
        DBG("Destructor handle=" << handle);
    }
};

static std::mutex g_threadsMtx;
static std::unordered_map<int, XThread*> g_threads;

static std::mutex g_eventMtx;
static std::unordered_map<int, std::unordered_map<std::string, void*>> g_threadEvents;

//==============================================================================
//  triggerEvent implementation
//==============================================================================
static void triggerEvent(int handle, const std::string& eventName, const char* param) {
    void* cb = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        auto it = g_threadEvents.find(handle);
        if (it != g_threadEvents.end()) {
            auto jt = it->second.find(eventName);
            if (jt != it->second.end()) cb = jt->second;
        }
    }
    if (!cb) return;
    using CB = void(*)(const char*);
    char* data = strdup(param ? param : "");
    DBG("Invoking Run for handle=" << handle);
    ((CB)cb)(data);
    free(data);
}

extern "C" {

//------------------------------------------------------------------------------
//  Constructor / Destructor
//------------------------------------------------------------------------------
XPLUGIN_API int Constructor() {
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<int> dist(10000000,99999999);
    int h;
    {
        std::lock_guard<std::mutex> lk(g_threadsMtx);
        do { h = dist(rng); } while (g_threads.count(h));
        g_threads[h] = new XThread(h);
    }
    return h;
}

XPLUGIN_API void Close(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    auto it = g_threads.find(handle);
    if (it != g_threads.end()) {
        delete it->second;
        g_threads.erase(it);
        std::lock_guard<std::mutex> lk2(g_eventMtx);
        g_threadEvents.erase(handle);
        DBG("Closed handle=" << handle);
    }
}

//------------------------------------------------------------------------------
//  Event-property getter: OnRun
//------------------------------------------------------------------------------
XPLUGIN_API const char* OnRun_GET(int handle) {
    std::string s = "XThread:" + std::to_string(handle) + ":Run";
    return strdup(s.c_str());
}

//------------------------------------------------------------------------------
//  Register callback
//------------------------------------------------------------------------------
XPLUGIN_API bool XThread_SetEventCallback(int handle,
                                          const char* eventName,
                                          void* callback) {
    {
        std::lock_guard<std::mutex> lk(g_threadsMtx);
        if (!g_threads.count(handle)) return false;
    }
    std::string key = eventName ? eventName : "";
    auto pos = key.rfind(':');
    if (pos != std::string::npos) key.erase(0, pos+1);
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        g_threadEvents[handle][key] = callback;
    }
    return true;
}

//------------------------------------------------------------------------------
//  Tag property
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Tag_SET(int handle, const char* v) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end()) {
        it->second->tag = v ? v : "";
    }
}
XPLUGIN_API const char* XThread_Tag_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end()) {
        return strdup(it->second->tag.c_str());
    }
    return strdup("");
}

//------------------------------------------------------------------------------
//  ThreadID (read-only)
//------------------------------------------------------------------------------
XPLUGIN_API int XThread_ThreadID_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end()) {
        if (it->second->thr.joinable()) {
            auto id = it->second->thr.get_id();
            return (int)std::hash<std::thread::id>{}(id);
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
//  ThreadState (read-only)
//------------------------------------------------------------------------------
XPLUGIN_API int XThread_ThreadState_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end())
        return it->second->threadState.load();
    return 4;
}

//------------------------------------------------------------------------------
//  Type property
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Type_SET(int handle, int v) {
    if (v < 0) v = 0;
    if (v > 1) v = 1;
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end())
        it->second->threadType = v;
}
XPLUGIN_API int XThread_Type_GET(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end())
        return it->second->threadType.load();
    return 0;
}

//------------------------------------------------------------------------------
//  Pause
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Pause(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end())
        it->second->threadState = 2;
}

//------------------------------------------------------------------------------
//  Resume
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Resume(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    if (auto it = g_threads.find(handle); it != g_threads.end())
        it->second->threadState = 0;
}

//------------------------------------------------------------------------------
//  Sleep(milliseconds, wakeEarly=false)
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Sleep(int handle, int ms, bool /*wakeEarly*/) {
    {
        std::lock_guard<std::mutex> lk(g_threadsMtx);
        if (auto it = g_threads.find(handle); it != g_threads.end())
            it->second->threadState = 3;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    {
        std::lock_guard<std::mutex> lk(g_threadsMtx);
        if (auto it = g_threads.find(handle); it != g_threads.end())
            it->second->threadState = 0;
    }
}

//------------------------------------------------------------------------------
//  Start
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Start(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    auto it = g_threads.find(handle);
    if (it == g_threads.end()) return;
    auto thrPtr = it->second;
    if (thrPtr->thr.joinable()) return;  // already started

    thrPtr->thr = std::thread([thrPtr]() {
        thrPtr->threadState = 0;                      // mark Running
        ::triggerEvent(thrPtr->handle, "Run", nullptr);
        thrPtr->threadState = 4;                      // mark NotRunning
    });
}

//------------------------------------------------------------------------------
//  Stop
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_Stop(int handle) {
    std::lock_guard<std::mutex> lk(g_threadsMtx);
    auto it = g_threads.find(handle);
    if (it != g_threads.end() && it->second->thr.joinable()) {
        it->second->thr.join();
        it->second->threadState = 4; // NotRunning
    }
}

//------------------------------------------------------------------------------
//  YieldToNext
//------------------------------------------------------------------------------
XPLUGIN_API void XThread_YieldToNext(int /*handle*/) {
    std::this_thread::yield();
}

//------------------------------------------------------------------------------
//  ClassDefinition boilerplate
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
    { "Tag",        "string",  (void*)XThread_Tag_GET,         (void*)XThread_Tag_SET       },
    { "ThreadID",   "integer", (void*)XThread_ThreadID_GET,    nullptr                      },
    { "ThreadState","integer", (void*)XThread_ThreadState_GET, nullptr                      },
    { "Type",       "integer", (void*)XThread_Type_GET,        (void*)XThread_Type_SET      },
    { "OnRun",      "string",  (void*)OnRun_GET,               nullptr                      }
};

static ClassEntry methods[] = {
    { "XThread_SetEventCallback", (void*)XThread_SetEventCallback, 3,
      { "integer","string","pointer" }, "boolean" },
    { "Pause",       (void*)XThread_Pause,       1, {"integer"}, "void"   },
    { "Resume",      (void*)XThread_Resume,      1, {"integer"}, "void"   },
    { "Sleep",       (void*)XThread_Sleep,       3, {"integer","integer","boolean"}, "void" },
    { "Start",       (void*)XThread_Start,       1, {"integer"}, "void"   },
    { "Stop",        (void*)XThread_Stop,        1, {"integer"}, "void"   },
    { "YieldToNext", (void*)XThread_YieldToNext, 1, {"integer"}, "void"   }
};

static ClassDefinition classDef = {
    "XThread",
    sizeof(XThread),
    (void*)Constructor,
    props,   sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &classDef;
}

} // extern "C"
