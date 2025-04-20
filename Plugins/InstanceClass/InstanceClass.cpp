// InstanceClass.cpp
// Created by Matthew A Combatti  
// Simulanics Technologies and Xojo Developers Studio  
// https://www.simulanics.com  
// https://www.xojostudio.org  
// DISCLAIMER: Simulanics Technologies and Xojo Developers Studio are not affiliated with Xojo, Inc.
// -----------------------------------------------------------------------------  
// Copyright (c) 2025 Simulanics Technologies and Xojo Developers Studio  
//  
// Permission is hereby granted, free of charge, to any person obtaining a copy  
// of this software and associated documentation files (the "Software"), to deal  
// in the Software without restriction, including without limitation the rights  
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
// copies of the Software, and to permit persons to whom the Software is  
// furnished to do so, subject to the following conditions:  
//  
// The above copyright notice and this permission notice shall be included in all  
// copies or substantial portions of the Software.  
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  
// SOFTWARE.
// ----------------------------------------------------------------------------- 
/*  ── Build commands ────────────────────────────────────────────────────────────
  Windows (MinGW‑w64)
    g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ ^
        -o InstanceClass.dll InstanceClass.cpp -pthread

  Linux
    g++ -std=c++17 -shared -fPIC -o libInstanceClass.so InstanceClass.cpp -pthread

  macOS
    g++ -std=c++17 -dynamiclib -o libInstanceClass.dylib InstanceClass.cpp -pthread
*/

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <string>
#include <cstdlib>
#include <cstring>     // <-- strdup
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>    // <-- debug output
#include <iomanip>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup          // Windows uses _strdup
#else
  #include <unistd.h>
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

//==============================================================================
//  Debug helpers
//==============================================================================
#define DBG_PREFIX "InstanceClass DEBUG: "
#define DBG(msg)  do { std::cout << DBG_PREFIX << msg << std::endl; } while (0)

// forward declaration so the worker thread can invoke it
static void triggerEvent(int handle,
                         const std::string& eventName,
                         const char* param);

//==============================================================================
//  MyInstance ‑ one per script object
//==============================================================================
class MyInstance
{
private:
    double              value{};
    std::mutex          mtx;
    std::thread         eventThread;
    std::atomic<bool>   running{false};
    int                 handle;

public:
    explicit MyInstance(int h) : handle(h)
    {
        DBG("Constructor(): Allocating new instance with handle " << handle);
        running.store(true);

        // fire an OnTrigger every 2 seconds
        eventThread = std::thread([this]()
        {
            DBG("Event thread started for handle " << handle);
            while (running.load())
            {
                std::this_thread::sleep_for(std::chrono::seconds(2));

                const char* payload = "2 second trigger";
                DBG("Event thread for handle " << handle
                    << " is about to fire OnTrigger with payload \""
                    << payload << '"');

                triggerEvent(handle, "OnTrigger", payload);
            }
            DBG("Event thread exiting for handle " << handle);
        });
    }

    ~MyInstance()
    {
        DBG("~MyInstance() for handle " << handle);
        running.store(false);
        if (eventThread.joinable())
            eventThread.join();
    }

    void Value_SET(double v)
    {
        std::lock_guard<std::mutex> lock(mtx);
        value = v;
        DBG("handle " << handle << " Value_SET(" << v << ')');
    }

    double Value_GET()
    {
        std::lock_guard<std::mutex> lock(mtx);
        DBG("handle " << handle << " Value_GET -> " << value);
        return value;
    }
};

//==============================================================================
//  Global containers
//==============================================================================
static std::mutex                                  g_instancesMtx;
static std::unordered_map<int, MyInstance*>        g_instances;
static std::atomic<int>                            g_nextHandle{1};

static std::mutex                                  g_eventMtx;
/*   handle  ─┬─> { "OnTrigger" → callbackPtr, … } */
static std::unordered_map<int,
        std::unordered_map<std::string, void*>>    g_eventCallbacks;

//==============================================================================
//  House‑keeping helpers
//==============================================================================
static void CleanupInstances()
{
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    DBG("CleanupInstances() called.");
    for (auto& p : g_instances)
        delete p.second;
    g_instances.clear();

    std::lock_guard<std::mutex> lk2(g_eventMtx);
    g_eventCallbacks.clear();
}

//==============================================================================
//  Event‑property getter (returns “MyInstance:<handle>:OnTrigger”)
//==============================================================================
extern "C" XPLUGIN_API
const char* OnTrigger_GET(int handle)
{
    std::string res = "MyInstance:" + std::to_string(handle) + ":OnTrigger";
    DBG("OnTrigger_GET for handle " << handle << " -> \"" << res << '"');
    return strdup(res.c_str());
}

//==============================================================================
//  Register callback
//==============================================================================
extern "C" XPLUGIN_API
bool MyInstance_SetEventCallback(int handle,
                                 const char* eventName,
                                 void* callback)
{
    {
        std::lock_guard<std::mutex> lk(g_instancesMtx);
        if (g_instances.find(handle) == g_instances.end())
            return false;           // invalid handle
    }

    std::string key(eventName ? eventName : "");
    auto pos = key.rfind(':');
    if (pos != std::string::npos)           // strip “MyInstance:##:”
        key.erase(0, pos + 1);

    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        g_eventCallbacks[handle][key] = callback;
    }

    DBG("SetEventCallback: handle=" << handle
        << " key=\"" << key
        << "\" callback=" << callback);
    return true;
}

//==============================================================================
//  triggerEvent  (worker thread → script)
//==============================================================================
// Called by the event thread in MyInstance.
static void triggerEvent(int handle,
                         const std::string& eventName,
                         const char* param)
{
    // ---- 1) grab the callback pointer while holding the lock ----------
    void* cbPtr = nullptr;
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        auto hit = g_eventCallbacks.find(handle);
        if (hit != g_eventCallbacks.end())
        {
            auto eit = hit->second.find(eventName);
            if (eit != hit->second.end())
                cbPtr = eit->second;
        }
    }   // ← mutex released here

    if (!cbPtr)
    {
        DBG("triggerEvent: no callback registered for handle "
            << handle << " event \"" << eventName << '"');
        return;
    }

    DBG("triggerEvent: handle=" << handle
        << " eventName=\"" << eventName
        << "\" param=" << (param ? param : "(null)")
        << " cbPtr=" << cbPtr);

    // ---- 2) finally invoke the callback *outside* the lock ------------
    if (eventName == "OnTrigger")
    {
        using TriggerCB = void (*)(const char*);
        auto cb = reinterpret_cast<TriggerCB>(cbPtr);

        // make a durable copy of the payload, replicating TimeTicker’s pattern
        char* safeStr = strdup(param ? param : "");
        DBG("triggerEvent: invoking OnTrigger callback @" << cb);
        cb(safeStr);
        free(safeStr);
    }
}

//==============================================================================
//  ── Exported C interface ──
//==============================================================================
extern "C" {

// ───────────────────────── Constructor / Destructor
XPLUGIN_API int Constructor()
{
    int h = g_nextHandle.fetch_add(1);
    auto* obj = new MyInstance(h);

    std::lock_guard<std::mutex> lk(g_instancesMtx);
    g_instances[h] = obj;
    return h;
}

XPLUGIN_API void Close(int handle)
{
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(handle);
    if (it != g_instances.end())
    {
        delete it->second;
        g_instances.erase(it);
        DBG("Close(): instance " << handle << " destroyed");
    }
}

// ───────────────────────── Properties
XPLUGIN_API void Value_SET(int handle, double v)
{
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(handle);
    if (it != g_instances.end())
        it->second->Value_SET(v);
}

XPLUGIN_API double Value_GET(int handle)
{
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(handle);
    return (it != g_instances.end()) ? it->second->Value_GET() : 0.0;
}

// ───────────────────────── Other methods
XPLUGIN_API double MultiplyTwoNumbers(int handle, double a, double b)
{
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if (g_instances.find(handle) == g_instances.end())
        return 0.0;

    DBG("MultiplyTwoNumbers called with " << a << " * " << b);
    return a * b;
}

// ───────────────────────── Class definition for the VM
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
    { "Value",      "double", (void*)Value_GET, (void*)Value_SET },
    { "OnTrigger",  "string", (void*)OnTrigger_GET, nullptr      }
};

static ClassEntry methods[] = {
    { "MultiplyTwoNumbers",        (void*)MultiplyTwoNumbers,         3,
      { "integer", "double", "double" }, "double" },
    { "MyInstance_SetEventCallback",(void*)MyInstance_SetEventCallback,3,
      { "integer","string","pointer" }, "boolean" }
};

static ClassConstant consts[] = {
    { "kMaxValue as Integer = 100" }
};

static ClassDefinition classDef = {
    "MyInstance",
    sizeof(MyInstance),
    (void*)Constructor,
    props,   sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    consts,  sizeof(consts)/sizeof(consts[0])
};

XPLUGIN_API ClassDefinition* GetClassDefinition() { return &classDef; }

} // extern "C"

//==============================================================================
//  Library unload hook
//==============================================================================
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_DETACH)
        CleanupInstances();
    return TRUE;
}
#else
__attribute__((destructor))
static void onUnload() { CleanupInstances(); }
#endif
