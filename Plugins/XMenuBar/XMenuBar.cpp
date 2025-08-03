/*

  XMenuBar.cpp
  CrossBasic Plugin: XMenuBar                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <string>
#include <cstdlib>
#include <random>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// Forward‑declare helper to get XWindow HWND (from XWindow.dll)
using XWindow_GetHWND_Fn = HWND(*)(int);
static XWindow_GetHWND_Fn sGetXWindowHWND = nullptr;
static HWND GetXWindowHWND(int winHandle) {
    if (!sGetXWindowHWND) {
        HMODULE m = GetModuleHandleA("XWindow.dll");
        if (m) sGetXWindowHWND = (XWindow_GetHWND_Fn)GetProcAddress(m, "XWindow_GetHWND");
    }
    return sGetXWindowHWND ? sGetXWindowHWND(winHandle) : nullptr;
}

//------------------------------------------------------------------------------
// XMenuBar instance representation
//------------------------------------------------------------------------------
class XMenuBar {
public:
    int     handle;
    int     parentHandle{0};
#ifdef _WIN32
    HMENU   hMenu{nullptr};
    HWND    hwnd{nullptr};
    bool    created{false};
#endif

    explicit XMenuBar(int h) : handle(h) {}
    ~XMenuBar() {
#ifdef _WIN32
        if (created && hMenu) {
            DestroyMenu(hMenu);
        }
#endif
    }
};

//------------------------------------------------------------------------------
// Globals
//------------------------------------------------------------------------------
static std::mutex                               gInstMx;
static std::unordered_map<int, XMenuBar*>       gInstances;
static std::mt19937                             gRng(std::random_device{}());
static std::uniform_int_distribution<int>       gDist(10000000, 99999999);

//------------------------------------------------------------------------------
// Cleanup on unload
//------------------------------------------------------------------------------
static void CleanupAll() {
    std::lock_guard<std::mutex> lk(gInstMx);
    for (auto& kv : gInstances) {
        delete kv.second;
    }
    gInstances.clear();
}

//------------------------------------------------------------------------------
// Exported helpers for XMenuItem.cpp to call into this DLL
//------------------------------------------------------------------------------
extern "C" {

// Return the HMENU for a given XMenuBar handle
XPLUGIN_API HMENU XMenuBar_GetHMenu(int h) {
    std::lock_guard<std::mutex> lk(gInstMx);
    auto it = gInstances.find(h);
    return (it != gInstances.end()) ? it->second->hMenu : nullptr;
}

// Return the HWND for a given XMenuBar handle
XPLUGIN_API HWND XMenuBar_GetHWND(int h) {
    std::lock_guard<std::mutex> lk(gInstMx);
    auto it = gInstances.find(h);
    return (it != gInstances.end()) ? it->second->hwnd : nullptr;
}

} // extern "C"

extern "C" {

//------------------------------------------------------------------------------
// Constructor / Close
//------------------------------------------------------------------------------
XPLUGIN_API int Constructor() {
    std::lock_guard<std::mutex> lk(gInstMx);
    int h;
    do {
        h = gDist(gRng);
    } while (gInstances.count(h));
    gInstances[h] = new XMenuBar(h);
    return h;
}

XPLUGIN_API void Close(int h) {
    std::lock_guard<std::mutex> lk(gInstMx);
    auto it = gInstances.find(h);
    if (it != gInstances.end()) {
        delete it->second;
        gInstances.erase(it);
    }
}

//------------------------------------------------------------------------------
// Parent property
//------------------------------------------------------------------------------
XPLUGIN_API void XMenuBar_Parent_SET(int h, int ph) {
    std::lock_guard<std::mutex> lk(gInstMx);
    auto it = gInstances.find(h);
    if (it == gInstances.end()) return;
    auto inst = it->second;
    inst->parentHandle = ph;
#ifdef _WIN32
    // Attach the menu bar to the window
    HWND wh = GetXWindowHWND(ph);
    if (!wh) return;
    inst->hwnd = wh;
    if (!inst->created) {
        inst->hMenu = CreateMenu();
        SetMenu(wh, inst->hMenu);
        DrawMenuBar(wh);
        inst->created = true;
    }
#endif
}

XPLUGIN_API int XMenuBar_Parent_GET(int h) {
    std::lock_guard<std::mutex> lk(gInstMx);
    auto it = gInstances.find(h);
    return (it != gInstances.end()) ? it->second->parentHandle : 0;
}

//------------------------------------------------------------------------------
// Handle property (so menubar.Handle is a real integer)
//------------------------------------------------------------------------------
XPLUGIN_API int XMenuBar_Handle_GET(int h) {
    return h;
}

//------------------------------------------------------------------------------
// Class definition boilerplate
//------------------------------------------------------------------------------
typedef struct { const char* name; const char* type; void* getter; void* setter; } ClassProperty;
typedef struct { const char* declaration; } ClassConstant;
typedef struct {
    const char*     className;
    size_t          classSize;
    void*           constructor;
    ClassProperty*  properties;
    size_t          propertiesCount;
    void*           methods;      
    size_t          methodsCount; 
    ClassConstant*  constants;    
    size_t          constantsCount;
} ClassDefinition;

static ClassProperty props[] = {
    { "Handle", "integer", (void*)XMenuBar_Handle_GET,   nullptr },
    { "Parent", "integer", (void*)XMenuBar_Parent_GET,   (void*)XMenuBar_Parent_SET }
};

static ClassDefinition classDef = {
    "XMenuBar",
    sizeof(XMenuBar),
    (void*)Constructor,
    props, sizeof(props)/sizeof(props[0]),
    nullptr, 0,
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &classDef;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_DETACH) CleanupAll();
    return TRUE;
}
#else
__attribute__((destructor))
static void onUnload() {
    CleanupAll();
}
#endif

} // extern "C"
