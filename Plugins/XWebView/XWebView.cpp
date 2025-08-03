/*

  XWebView.cpp
  CrossBasic Plugin: XWebView                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
//
// Wrapper around https://github.com/webview/webview  
// Implements: Parent, Left/Top/Width/Height, LoadURL/HTML/Page, Refresh,  
//             GoBack, GoForward, ExecuteJavaScript(+Sync), URL,  
//             LockTop/Left/Bottom/Right anchoring  
// ----------------------------------------------------------------------------

#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <algorithm>

#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #define UNICODE
  #define _UNICODE
  #include <windows.h>
  #include <uxtheme.h>
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// ─── webview single-header ───────────────────────────────────────────────────
#define WEBVIEW_DEBUG 0
#define WEBVIEW_IMPLEMENTATION
#include "webview.h"

// ═════════ helper: read file into string ════════════════════════════════════
static std::string readFile(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream os;
    os << f.rdbuf();
    return os.str();
}

// ═════════ JavaScript sync support ══════════════════════════════════════════
struct JSContext {
    std::mutex m;
    std::condition_variable cv;
    bool done = false;
    std::string result;
};
static void js_callback(const char*, const char* req, void* arg) {
    auto ctx = static_cast<JSContext*>(arg);
    {
        std::lock_guard<std::mutex> lk(ctx->m);
        ctx->result = req ? req : "";
        ctx->done = true;
    }
    ctx->cv.notify_one();
}
static void unbind_callback(webview_t w, void* arg) {
    char* id = static_cast<char*>(arg);
    webview_unbind(w, id);
    free(id);
}

#ifdef _WIN32
// Forward-declare XWebView so our anchor structures can mention it
class XWebView;

// One AnchorSet per parent window, holds pointers to all anchored children
struct AnchorSet {
    HWND                      parent{};
    std::vector<XWebView*>    children;
};
static std::unordered_map<HWND, AnchorSet> gAnchors;

// Forward-declare ParentSizeProc so XWebView can refer to it
static LRESULT CALLBACK ParentSizeProc(
    HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR, DWORD_PTR
);

// Helper: get the HWND of the XWindow parent
static HWND GetParentHWND(int xojoHandle) {
    using Fn = HWND(*)(int);
    static Fn fn = nullptr;
    if (!fn) {
        HMODULE m = GetModuleHandleA("XWindow.dll");
        if (m) fn = (Fn)GetProcAddress(m, "XWindow_GetHWND");
    }
    return fn ? fn(xojoHandle) : nullptr;
}
#endif

// ═════════ XWebView class ══════════════════════════════════════════════════
class XWebView {
public:
    int             handle{};
    int             x{0}, y{0}, w{800}, h{600};
    int             parentHandle{0};
    webview_t       wv{nullptr};
    std::thread     loop;
    std::atomic<bool> running{false};
    std::string     currentURL;

    // anchoring flags + offsets
    bool LockTop{false}, LockLeft{false},
         LockBottom{false}, LockRight{false};
    int  rightOffset{0}, bottomOffset{0};

    explicit XWebView(int hdl) : handle(hdl) {
        wv = webview_create(0, nullptr);
        webview_set_size(wv, w, h, WEBVIEW_HINT_NONE);
        running = true;
        loop = std::thread([this] { webview_run(wv); });
    }
    ~XWebView() {
        running = false;
        if (wv) webview_terminate(wv);
        if (loop.joinable()) loop.join();
        if (wv) webview_destroy(wv);
    }

    // Apply geometry immediately
    void applyGeom() {
#ifdef _WIN32
        HWND hwnd = (HWND)webview_get_window(wv);
        if (hwnd) {
            SetWindowPos(hwnd, nullptr, x, y, w, h,
                         SWP_NOZORDER|SWP_SHOWWINDOW);
        }
#else
        webview_set_size(wv, w, h, WEBVIEW_HINT_NONE);
#endif
    }

    // Parent/anchoring logic
    void applyParent() {
#ifdef _WIN32
        if (!parentHandle) return;
        HWND hwndParent = GetParentHWND(parentHandle);
        HWND hwnd       = (HWND)webview_get_window(wv);
        if (hwnd && hwndParent) {
            LONG style = GetWindowLongW(hwnd, GWL_STYLE);
            style &= ~(WS_POPUP|WS_CAPTION|WS_THICKFRAME|
                       WS_BORDER|WS_DLGFRAME);
            style |= WS_CHILD;
            SetWindowLongW(hwnd, GWL_STYLE, style);

            LONG ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
            ex &= ~(WS_EX_DLGMODALFRAME|
                    WS_EX_CLIENTEDGE|
                    WS_EX_STATICEDGE);
            SetWindowLongW(hwnd, GWL_EXSTYLE, ex);

            SetParent(hwnd, hwndParent);
            SetWindowPos(hwnd, nullptr, x, y, w, h,
                         SWP_NOZORDER|SWP_SHOWWINDOW|SWP_FRAMECHANGED);

            registerAnchor(hwndParent);
        }
#endif
    }

    // synchronous JS evaluation
    std::string executeSync(const std::string& js) {
        JSContext ctx;
        std::string cbName = "_xwv" + std::to_string(handle);
        char* id = strdup(cbName.c_str());
        webview_bind(wv, id, js_callback, &ctx);
        std::string script =
          std::string(id) + "( JSON.stringify((function(){ " +
          js + " })()) );";
        webview_eval(wv, script.c_str());
        {
            std::unique_lock<std::mutex> lk(ctx.m);
            ctx.cv.wait_for(lk, std::chrono::seconds(5),
                            [&]{ return ctx.done; });
        }
        webview_dispatch(wv, unbind_callback, id);
        return ctx.result;
    }

#ifdef _WIN32
    // Internal: register this instance in the parent's anchor list
    void registerAnchor(HWND parent) {
        bool anchored = LockTop||LockLeft||LockBottom||LockRight;
        if (!anchored) return;

        RECT prc, crc;
        GetClientRect(parent, &prc);

        HWND hwnd = (HWND)webview_get_window(wv);
        GetWindowRect(hwnd, &crc);
        MapWindowPoints(HWND_DESKTOP, parent, (POINT*)&crc, 2);

        if (LockRight)  rightOffset  = prc.right  - crc.right;
        if (LockBottom) bottomOffset = prc.bottom - crc.bottom;

        auto &aset = gAnchors[parent];
        aset.parent = parent;
        aset.children.push_back(this);
        if (aset.children.size() == 1) {
            SetWindowSubclass(parent,
                              ParentSizeProc,
                              0xFEED,
                              0);
        }
    }
#endif
};

// ═════════ ParentSizeProc implementation (after XWebView) ═══════════════════
#ifdef _WIN32
static LRESULT CALLBACK ParentSizeProc(
    HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
    UINT_PTR, DWORD_PTR
) {
    if (msg == WM_SIZE) {
        RECT prc;
        GetClientRect(hWnd, &prc);
        auto it = gAnchors.find(hWnd);
        if (it != gAnchors.end()) {
            for (auto *vw : it->second.children) {
                HWND ctl = (HWND)webview_get_window(vw->wv);
                if (!ctl) continue;

                int nx = vw->x, ny = vw->y;
                int nw = vw->w, nh = vw->h;

                // Horizontal anchoring
                if (vw->LockLeft && vw->LockRight) {
                    nw = prc.right - vw->rightOffset - vw->x;
                }
                else if (vw->LockRight) {
                    nx = prc.right - vw->rightOffset - vw->w;
                }

                // Vertical anchoring
                if (vw->LockTop && vw->LockBottom) {
                    nh = prc.bottom - vw->bottomOffset - vw->y;
                }
                else if (vw->LockBottom) {
                    ny = prc.bottom - vw->bottomOffset - vw->h;
                }

                SetWindowPos(ctl, nullptr, nx, ny, nw, nh,
                             SWP_NOZORDER|SWP_SHOWWINDOW);
                vw->x = nx; vw->y = ny;
                vw->w = nw; vw->h = nh;
            }
        }
    }
    return DefSubclassProc(hWnd, msg, wp, lp);
}
#endif

// ═════════ global bookkeeping ═══════════════════════════════════════════════
static std::mutex gMx;
static std::unordered_map<int, XWebView*> gInst;
static std::mt19937 rng(std::random_device{}());
static std::uniform_int_distribution<int> dist(10000000, 99999999);

// ═════════ exported C interface ════════════════════════════════════════════
extern "C" {

// Constructor / Destructor
XPLUGIN_API int Constructor() {
    std::lock_guard<std::mutex> lk(gMx);
    int h;
    do { h = dist(rng); } while (gInst.count(h));
    gInst[h] = new XWebView(h);
    return h;
}
XPLUGIN_API void Close(int h) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gInst.find(h);
    if (it != gInst.end()) {
        delete it->second;
        gInst.erase(it);
    }
}

// ── geometry ═══════════════════════════════════════════════════════════════
#define PROP_INT(NAME,FIELD) \
XPLUGIN_API void XWebView_##NAME##_SET(int h,int v){ \
    std::lock_guard<std::mutex> lk(gMx); auto it=gInst.find(h); \
    if(it!=gInst.end()){ it->second->FIELD=v; it->second->applyGeom(); }} \
XPLUGIN_API int XWebView_##NAME##_GET(int h){ \
    std::lock_guard<std::mutex> lk(gMx); auto it=gInst.find(h); \
    return it!=gInst.end()?it->second->FIELD:0; }

PROP_INT(Left,  x)
PROP_INT(Top,   y)
PROP_INT(Width, w)
PROP_INT(Height,h)
#undef PROP_INT

// ── Parent ════════════════════════════════════════════════════════════════
XPLUGIN_API void XWebView_Parent_SET(int h,int ph){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if(it!=gInst.end()){
        it->second->parentHandle = ph;
        it->second->applyParent();
    }
}
XPLUGIN_API int XWebView_Parent_GET(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    return it!=gInst.end()?it->second->parentHandle:0;
}

// ── navigation ════════════════════════════════════════════════════════════
XPLUGIN_API void XWebView_LoadURL(int h,const char* url){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end() && url) {
        webview_navigate(it->second->wv, url);
        it->second->currentURL = url;
    }
}
XPLUGIN_API void XWebView_LoadHTML(int h,const char* html){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end() && html) {
        std::string data = std::string("data:text/html,") + html;
        webview_navigate(it->second->wv, data.c_str());
        it->second->currentURL = data;
    }
}
XPLUGIN_API void XWebView_LoadPage(int h,const char* path){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end() && path) {
        std::string html = readFile(path);
        if (!html.empty()) {
            std::string data = std::string("data:text/html,") + html;
            webview_navigate(it->second->wv, data.c_str());
            it->second->currentURL = std::string("file://") + path;
        }
    }
}
XPLUGIN_API void XWebView_Refresh(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end())
        webview_eval(it->second->wv, "location.reload()");
}
XPLUGIN_API void XWebView_GoBack(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end())
        webview_eval(it->second->wv, "history.back()");
}
XPLUGIN_API void XWebView_GoForward(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end())
        webview_eval(it->second->wv, "history.forward()");
}
XPLUGIN_API void XWebView_ExecuteJavaScript(int h,const char* js){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it!=gInst.end() && js)
        webview_eval(it->second->wv, js);
}
XPLUGIN_API const char* XWebView_ExecuteJavaScriptSync(int h,const char* js){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    if (it==gInst.end() || !js) return strdup("");
    std::string r = it->second->executeSync(js);
    return strdup(r.c_str());
}
XPLUGIN_API const char* XWebView_URL_GET(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h);
    return it!=gInst.end()
       ? strdup(it->second->currentURL.c_str())
       : strdup("");
}

// ── anchoring boolean props ═══════════════════════════════════════════════
#define BOOL_PROP(NAME,FIELD) \
XPLUGIN_API void XWebView_##NAME##_SET(int h,bool v){ \
    std::lock_guard<std::mutex> lk(gMx); auto it=gInst.find(h); \
    if(it==gInst.end()) return; \
    auto *vw=it->second; bool old=vw->FIELD; \
    vw->FIELD = v; \
    if (vw->parentHandle && v!=old) vw->applyParent(); \
} \
XPLUGIN_API bool XWebView_##NAME##_GET(int h){ \
    std::lock_guard<std::mutex> lk(gMx); auto it=gInst.find(h); \
    return it!=gInst.end() ? it->second->FIELD : false; \
}

BOOL_PROP(LockTop,   LockTop)
BOOL_PROP(LockLeft,  LockLeft)
BOOL_PROP(LockRight, LockRight)
BOOL_PROP(LockBottom,LockBottom)
#undef BOOL_PROP

// ── Xojo class-definition tables ═════════════════════════════════════════
typedef struct{const char* n;const char* t;void* g;void* s;} PropDef;
typedef struct{const char* n;void* f;int a;const char* p[10];const char* r;} MethDef;
typedef struct{const char* d;} ConstDef;
typedef struct{ const char* name; size_t size; void* ctor;
                PropDef* props; size_t propsCount;
                MethDef* meths; size_t methCount;
                ConstDef* csts; size_t cstCount; } ClassDef;

static PropDef props[] = {
    {"Left","integer",(void*)XWebView_Left_GET,(void*)XWebView_Left_SET},
    {"Top","integer",(void*)XWebView_Top_GET,(void*)XWebView_Top_SET},
    {"Width","integer",(void*)XWebView_Width_GET,(void*)XWebView_Width_SET},
    {"Height","integer",(void*)XWebView_Height_GET,(void*)XWebView_Height_SET},
    {"Parent","integer",(void*)XWebView_Parent_GET,(void*)XWebView_Parent_SET},
    {"URL","string",(void*)XWebView_URL_GET,nullptr},
    {"LockTop","boolean",(void*)XWebView_LockTop_GET,(void*)XWebView_LockTop_SET},
    {"LockLeft","boolean",(void*)XWebView_LockLeft_GET,(void*)XWebView_LockLeft_SET},
    {"LockRight","boolean",(void*)XWebView_LockRight_GET,(void*)XWebView_LockRight_SET},
    {"LockBottom","boolean",(void*)XWebView_LockBottom_GET,(void*)XWebView_LockBottom_SET},
};
static MethDef meths[] = {
    {"LoadURL",(void*)XWebView_LoadURL,2,{"integer","string"},"void"},
    {"LoadHTML",(void*)XWebView_LoadHTML,2,{"integer","string"},"void"},
    {"LoadPage",(void*)XWebView_LoadPage,2,{"integer","string"},"void"},
    {"Refresh",(void*)XWebView_Refresh,1,{"integer"},"void"},
    {"GoBack",(void*)XWebView_GoBack,1,{"integer"},"void"},
    {"GoForward",(void*)XWebView_GoForward,1,{"integer"},"void"},
    {"ExecuteJavaScript",(void*)XWebView_ExecuteJavaScript,2,{"integer","string"},"void"},
    {"ExecuteJavaScriptSync",(void*)XWebView_ExecuteJavaScriptSync,2,{"integer","string"},"string"}
};
static ClassDef cls = {
    "XWebView",
    sizeof(XWebView),
    (void*)Constructor,
    props, sizeof(props)/sizeof(props[0]),
    meths, sizeof(meths)/sizeof(meths[0]),
    nullptr, 0
};

XPLUGIN_API ClassDef* GetClassDefinition() {
    return &cls;
}

// ── cleanup on unload ═════════════════════════════════════════════════════
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_DETACH) {
        std::lock_guard<std::mutex> lk(gMx);
        for (auto& kv : gInst) delete kv.second;
        gInst.clear();
    }
    return TRUE;
}
#else
__attribute__((destructor))
static void onUnload(){
    std::lock_guard<std::mutex> lk(gMx);
    for (auto& kv : gInst) delete kv.second;
    gInst.clear();
}
#endif

} // extern "C"
