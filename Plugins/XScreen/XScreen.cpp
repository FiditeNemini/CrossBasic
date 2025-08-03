/*

  XScreen.cpp
  CrossBasic Plugin: XScreen                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
  Windows (MinGW-w64)
    windres XScreen.rc -O coff -o XScreen.res
    g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ ^
        -o XScreen.dll XScreen.cpp XScreen.res -pthread -lgdi32 -luser32

  Linux
    g++ -std=c++17 -shared -fPIC -o libXScreen.so XScreen.cpp -pthread

  macOS
    g++ -std=c++17 -dynamiclib -o libXScreen.dylib XScreen.cpp -pthread
*/

#include <mutex>
#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <cstdlib>
#include <cstring>    // strdup
#include <random>


#ifdef _WIN32
  #include <windows.h>
  #include <shellscalingapi.h>
  #pragma comment(lib, "Shcore.lib")
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// forward declarations
static void CleanupInstances();
extern "C" XPLUGIN_API double XScreen_ScaleFactor_GET(int handle);

// -----------------------------------------------------------------------------
// Helper: duplicate and convert a wide‐string (UTF-16) to UTF-8, or just
// strdup on non-Win32 builds.
// -----------------------------------------------------------------------------
#ifdef _WIN32
static char* dupUtf8(const wchar_t* wstr) {
    // figure out how many bytes we need (including terminating NUL)
    int size = ::WideCharToMultiByte(
        CP_UTF8,            // convert to UTF-8
        0,
        wstr,
        -1,                 // NUL-terminated
        nullptr,
        0,
        nullptr,
        nullptr
    );
    if (size <= 0) return strdup("");
    char* buf = (char*)std::malloc(size);
    if (!buf)      return strdup("");
    ::WideCharToMultiByte(
        CP_UTF8,
        0,
        wstr,
        -1,
        buf,
        size,
        nullptr,
        nullptr
    );
    return buf;
}
// allow char* passthrough on Windows too:
static char* dupUtf8(const char* str) {
    return strdup(str);
}
#else
// non-Win32 just needs plain strdup
static char* dupUtf8(const char* str) {
    return strdup(str);
}
#endif


#ifdef _WIN32
static BOOL CALLBACK MonitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);
static void PopulateMonitors();
#endif

//------------------------------------------------------------------------------
//  XScreen instance
//------------------------------------------------------------------------------
class XScreen {
public:
    int       handle;
    int       index;
#ifdef _WIN32
    HMONITOR  hmon;
    XScreen(int h, int idx, HMONITOR m) : handle(h), index(idx), hmon(m) {}
#else
    XScreen(int h, int idx) : handle(h), index(idx) {}
#endif
    ~XScreen() = default;
};

//------------------------------------------------------------------------------
//  Globals
//------------------------------------------------------------------------------
static std::mutex g_instances_mtx;
static std::unordered_map<int, XScreen*> g_instances;
static std::vector<
#ifdef _WIN32
    HMONITOR
#else
    int
#endif
> g_monitors;
static std::atomic<bool> g_monitors_populated{false};
static std::mt19937 g_rng(std::random_device{}());
static std::uniform_int_distribution<int> g_handle_dist(10000000,99999999);
static std::mutex g_event_mtx;
static std::unordered_map<int,std::unordered_map<std::string,void*>> g_eventCallbacks;

//------------------------------------------------------------------------------
//  PopulateMonitors (Win32)
//------------------------------------------------------------------------------
#ifdef _WIN32
static BOOL CALLBACK MonitorEnumProc(HMONITOR m, HDC, LPRECT, LPARAM) {
    g_monitors.push_back(m);
    return TRUE;
}
static void PopulateMonitors() {
    if (!g_monitors_populated.exchange(true)) {
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
    }
}
#endif

//------------------------------------------------------------------------------
//  Helper to get instance
//------------------------------------------------------------------------------
static XScreen* GetInstance(int h) {
    std::lock_guard<std::mutex> lk(g_instances_mtx);
    auto it = g_instances.find(h);
    return it != g_instances.end() ? it->second : nullptr;
}

//------------------------------------------------------------------------------
//  Constructor (dummy, the real work is in DisplayAt)
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API
int Constructor() {
    // scripting can ignore this and call DisplayAt
    return 0;
}

//------------------------------------------------------------------------------
//  Shared properties
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API
int XScreen_DisplayCount_GET(int) {
#ifdef _WIN32
    PopulateMonitors();
    return (int)g_monitors.size();
#else
    return 1;
#endif
}
extern "C" XPLUGIN_API
int XScreen_LastDisplayIndex_GET(int h) {
    int c = XScreen_DisplayCount_GET(h);
    return c>0 ? c-1 : 0;
}

//------------------------------------------------------------------------------
//  Shared method DisplayAt(index) → new instance
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API
int XScreen_DisplayAt(int, int idx) {
#ifdef _WIN32
    PopulateMonitors();
    if (idx<0 || idx>=(int)g_monitors.size()) return 0;
    HMONITOR mon = g_monitors[idx];
#endif
    int h;
    {
        std::lock_guard<std::mutex> lk(g_instances_mtx);
        do { h = g_handle_dist(g_rng); }
        while (g_instances.count(h));
#ifdef _WIN32
        g_instances[h] = new XScreen(h, idx, mon);
#else
        g_instances[h] = new XScreen(h, idx);
#endif
    }
    return h;
}

//------------------------------------------------------------------------------
//  Instance properties
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API
int XScreen_AvailableHeight_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    return mi.rcWork.bottom - mi.rcWork.top;
#else
    return 0;
#endif
}
extern "C" XPLUGIN_API
int XScreen_AvailableLeft_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    return mi.rcWork.left;
#else
    return 0;
#endif
}
extern "C" XPLUGIN_API
int XScreen_AvailableTop_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    return mi.rcWork.top;
#else
    return 0;
#endif
}
extern "C" XPLUGIN_API
int XScreen_AvailableWidth_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    return mi.rcWork.right - mi.rcWork.left;
#else
    return 0;
#endif
}
extern "C" XPLUGIN_API
int XScreen_ColorDepth_GET(int) {
#ifdef _WIN32
    HDC dc = GetDC(NULL);
    int bpp = GetDeviceCaps(dc, BITSPIXEL);
    ReleaseDC(NULL, dc);
    return bpp;
#else
    return 0;
#endif
}

// scale factor (forward-declared above)
extern "C" XPLUGIN_API
double XScreen_ScaleFactor_GET(int) {
#ifdef _WIN32
    HDC dc = GetDC(NULL);
    int dpi = GetDeviceCaps(dc, LOGPIXELSX);
    ReleaseDC(NULL, dc);
    return dpi/96.0;
#else
    return 1.0;
#endif
}

// now Height can call ScaleFactor
extern "C" XPLUGIN_API
int XScreen_Height_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    int pixels = mi.rcMonitor.bottom - mi.rcMonitor.top;
    double sf = XScreen_ScaleFactor_GET(h);
    return int(pixels / (sf>0?sf:1.0));
#else
    return 0;
#endif
}

extern "C" XPLUGIN_API
int XScreen_Left_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    return mi.rcMonitor.left;
#else
    return 0;
#endif
}

extern "C" XPLUGIN_API
int XScreen_Top_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    return mi.rcMonitor.top;
#else
    return 0;
#endif
}

extern "C" XPLUGIN_API
int XScreen_Width_GET(int h) {
#ifdef _WIN32
    auto inst = GetInstance(h);
    if (!inst) return 0;
    MONITORINFO mi{ sizeof(mi) };
    GetMonitorInfo(inst->hmon, &mi);
    int pixels = mi.rcMonitor.right - mi.rcMonitor.left;
    double sf = XScreen_ScaleFactor_GET(h);
    return int(pixels / (sf>0?sf:1.0));
#else
    return 0;
#endif
}

// -----------------------------------------------------------------------------
// XScreen_Name_GET → produce “Display N” in UTF-8
// -----------------------------------------------------------------------------
extern "C" XPLUGIN_API
const char* XScreen_Name_GET(int h) {
    auto inst = GetInstance(h);
    if (!inst) return dupUtf8("");

#ifdef _WIN32
    // build a UTF‐16 string first
    wchar_t wbuf[64];
    _snwprintf_s(wbuf, 64, _TRUNCATE, L"Display %d", inst->index);
    return dupUtf8(wbuf);
#else
    // on non-Windows we assume char* is UTF-8
    char buf[64];
    snprintf(buf, sizeof(buf), "Display %d", inst->index);
    return dupUtf8(buf);
#endif
}

extern "C" XPLUGIN_API
const char* XScreen_ScreenDisplayMonitor_GET(int h) {
    auto inst = GetInstance(h);
    if (!inst) return strdup("");
#ifdef _WIN32
    MONITORINFOEXW mi{ sizeof(mi) };
    GetMonitorInfoW(inst->hmon, (MONITORINFO*)&mi);
    return dupUtf8(mi.szDevice);
#else
    return strdup("");
#endif
}

extern "C" XPLUGIN_API
const char* XScreen_Description_GET(int h) {
    auto inst = GetInstance(h);
    if (!inst) return strdup("");
#ifdef _WIN32
    MONITORINFOEXW mi{ sizeof(mi) };
    GetMonitorInfoW(inst->hmon, (MONITORINFO*)&mi);
    DISPLAY_DEVICEW dd{ sizeof(dd) };
    EnumDisplayDevicesW(mi.szDevice, 0, &dd, 0);
    return dupUtf8(dd.DeviceString);
#else
    return strdup("");
#endif
}


//------------------------------------------------------------------------------
//  Cleanup
extern "C" XPLUGIN_API
void Close(int h) {
    std::lock_guard<std::mutex> lk(g_instances_mtx);
    auto it = g_instances.find(h);
    if (it!=g_instances.end()) {
        delete it->second;
        g_instances.erase(it);
    }
}

//------------------------------------------------------------------------------
//  DllMain / unload hook
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if(reason==DLL_PROCESS_DETACH) CleanupInstances();
    return TRUE;
}
#else
__attribute__((destructor))
static void onUnload() { CleanupInstances(); }
#endif

//------------------------------------------------------------------------------
//  Definition of CleanupInstances
static void CleanupInstances() {
    std::lock_guard<std::mutex> lk(g_instances_mtx);
    for(auto& kv : g_instances) delete kv.second;
    g_instances.clear();
}

//------------------------------------------------------------------------------
//  ClassDefinition boilerplate
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
    { "Name",                 "string", (void*)XScreen_Name_GET,                 nullptr },
    { "ScreenDisplayMonitor", "string", (void*)XScreen_ScreenDisplayMonitor_GET, nullptr },
    { "Description",          "string", (void*)XScreen_Description_GET,          nullptr },
    { "AvailableHeight",      "integer",(void*)XScreen_AvailableHeight_GET,    nullptr },
    { "AvailableLeft",        "integer",(void*)XScreen_AvailableLeft_GET,      nullptr },
    { "AvailableTop",         "integer",(void*)XScreen_AvailableTop_GET,       nullptr },
    { "AvailableWidth",       "integer",(void*)XScreen_AvailableWidth_GET,     nullptr },
    { "ColorDepth",           "integer",(void*)XScreen_ColorDepth_GET,         nullptr },
    { "DisplayCount",         "integer",(void*)XScreen_DisplayCount_GET,       nullptr },
    { "Height",               "integer",(void*)XScreen_Height_GET,             nullptr },
    { "LastDisplayIndex",     "integer",(void*)XScreen_LastDisplayIndex_GET,   nullptr },
    { "Left",                 "integer",(void*)XScreen_Left_GET,               nullptr },
    { "ScaleFactor",          "double", (void*)XScreen_ScaleFactor_GET,          nullptr },
    { "Top",                  "integer",(void*)XScreen_Top_GET,                nullptr },
    { "Width",                "integer",(void*)XScreen_Width_GET,              nullptr }
};

static ClassEntry methods[] = {
    { "DisplayAt", (void*)XScreen_DisplayAt, 2, { "integer", "integer" }, "XScreen" }
};

static ClassDefinition classDef = {
    "XScreen",
    sizeof(XScreen),
    (void*)Constructor,
    props,   _countof(props),
    methods, _countof(methods),
    nullptr, 0
};

extern "C" XPLUGIN_API
ClassDefinition* GetClassDefinition() {
    return &classDef;
}
