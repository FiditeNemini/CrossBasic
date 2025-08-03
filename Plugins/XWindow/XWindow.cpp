/*

  XWindow.cpp
  CrossBasic Plugin: XWindow                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
   windres XWindow.rc -O coff -o XWindow.res
   g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ \
       -o XWindow.dll XWindow.cpp XWindow.res -pthread -s \
       -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -ole32
*/
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#include <random>

#ifdef _WIN32
  #include <windows.h>
  #include <dwmapi.h>
  #include <uxtheme.h>
  #pragma comment(lib, "dwmapi.lib")
  #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
    #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
  #endif
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

#ifndef WM_XWINDOW_OPEN
#define WM_XWINDOW_OPEN   (WM_APP + 0x100)
#endif


#ifdef _WIN32
  #include <gdiplus.h>
  #pragma comment(lib, "gdiplus.lib")
  using namespace Gdiplus;
  static ULONG_PTR gdiPlusToken = 0;
#endif

#define DBG_PREFIX "XWindow DEBUG: "
#define DBG(msg)  do { std::cout<<DBG_PREFIX<<msg<<std::endl; } while(0)

static void triggerEvent(int, const std::string&, const char*);
#ifdef _WIN32
static LRESULT CALLBACK XWindow_WndProc(HWND, UINT, WPARAM, LPARAM);
#endif

class XWindow
{
public:
    int                 handle;
    int                 winType{0};

#ifdef _WIN32
    HWND                hwnd{nullptr};
    std::thread         msgThread;
    unsigned int        bgColorVal{0x202020};
    COLORREF            bgColor{RGB(32,32,32)};
    HBRUSH              bgBrush{nullptr};

    // new style toggles (defaults as requested)
    bool HasCloseButton{true};
    bool HasMinimizeButton{true};
    bool HasMaximizeButton{true};
    bool HasFullScreenButton{false};
    bool HasTitleBar{true};
    bool Resizable{true};

    void updateWindowStyles()
    {
        DWORD style = WS_OVERLAPPED;
        if (HasTitleBar)    style |= WS_CAPTION;
        if (HasCloseButton) style |= WS_SYSMENU;
        if (HasMinimizeButton) style |= WS_MINIMIZEBOX;
        if (HasMaximizeButton || HasFullScreenButton) style |= WS_MAXIMIZEBOX;
        if (Resizable)      style |= WS_THICKFRAME;

        ::SetWindowLongPtrA(hwnd, GWL_STYLE, style);
        SetWindowPos(hwnd, NULL, 0,0,0,0,
                     SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
    }
#endif

    std::atomic<bool>   running{false};
    bool                openedFired{false};
    bool                inSizeMove{false};

    explicit XWindow(int h) : handle(h) {
        running = true;
#ifdef _WIN32
        registerClass();
        createWindowHidden();
        bgBrush = CreateSolidBrush(bgColor);
        msgThread = std::thread([this]{ messageLoop(); });
#endif
    }

    ~XWindow() {
        running = false;
#ifdef _WIN32
        if (hwnd) PostMessageA(hwnd, WM_CLOSE, 0, 0);
        if (msgThread.joinable()) msgThread.join();
        if (bgBrush) DeleteObject(bgBrush);
#endif
    }

#ifdef _WIN32
    void setBackgroundColor(unsigned int rgb) {
        bgColorVal = rgb & 0xFFFFFFu;
        BYTE r=(bgColorVal>>16)&0xFF, g=(bgColorVal>>8)&0xFF, b=bgColorVal&0xFF;
        bgColor = RGB(r,g,b);
        if(bgBrush) DeleteObject(bgBrush);
        bgBrush = CreateSolidBrush(bgColor);
        if(hwnd) {
            InvalidateRect(hwnd,nullptr,TRUE);
            UpdateWindow(hwnd);
        }
    }
    unsigned int getBackgroundColor() const { return bgColorVal; }

private:
    static void registerClass() {
        static std::once_flag once;
        std::call_once(once, []{
            WNDCLASSEXA wc{};
            wc.cbSize      = sizeof(wc);
            wc.lpfnWndProc = XWindow_WndProc;
            wc.hInstance   = GetModuleHandleA(NULL);
            wc.hCursor     = LoadCursorA(NULL, IDC_ARROW);
            wc.lpszClassName = "XWindowClass";
            RegisterClassExA(&wc);
        });
    }

    void createWindowHidden() {
        DWORD style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
        hwnd = CreateWindowExA(
            0, "XWindowClass", "XWindow",
            style, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
            NULL, NULL, GetModuleHandleA(NULL),
            reinterpret_cast<LPVOID>(this)
        );
        BOOL dark=TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    }

    void messageLoop() {
        MSG msg;
        while(running && GetMessageA(&msg,NULL,0,0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }

public:
    static void applyStyle(HWND hwnd, int t);
#endif
};

#ifdef _WIN32
static const struct{DWORD s,ex;} gStyles[6] = {
    {WS_OVERLAPPEDWINDOW,0},
    {WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_THICKFRAME,WS_EX_TOPMOST},
    {WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,WS_EX_DLGMODALFRAME},
    {WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,WS_EX_TOOLWINDOW},
    {WS_OVERLAPPED|WS_CAPTION,0},
    {WS_OVERLAPPED|WS_CAPTION,WS_EX_TOOLWINDOW|WS_EX_DLGMODALFRAME|WS_EX_LAYERED}
};
void XWindow::applyStyle(HWND hwnd,int t){
    if(t<0||t>5) t=0;
    SetWindowLongPtrA(hwnd,GWL_STYLE,gStyles[t].s);
    SetWindowLongPtrA(hwnd,GWL_EXSTYLE,gStyles[t].ex);
    SetWindowPos(hwnd,NULL,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
}
#endif

static std::mutex g_instancesMtx;
static std::unordered_map<int,XWindow*> g_instances;
static std::mt19937 g_rng(std::random_device{}());
static std::uniform_int_distribution<int> g_handleDist(10000000,99999999);
static std::mutex g_eventMtx;
static std::unordered_map<int,std::unordered_map<std::string,void*>> g_eventCallbacks;

static void CleanupInstances(){
    {
        std::lock_guard<std::mutex> lk(g_instancesMtx);
        for(auto &p:g_instances) delete p.second;
        g_instances.clear();
    }
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        g_eventCallbacks.clear();
    }
}

static void triggerEvent(int h,const std::string& e,const char* p){
    void* cb=nullptr;
    {
        std::lock_guard<std::mutex> lk(g_eventMtx);
        auto it=g_eventCallbacks.find(h);
        if(it!=g_eventCallbacks.end()){
            auto eit=it->second.find(e);
            if(eit!=it->second.end()) cb=eit->second;
        }
    }
    if(!cb) return;
    using CB=void(*)(const char*);
    char* dup=strdup(p?p:"");
    reinterpret_cast<CB>(cb)(dup);
    free(dup);
}

#ifdef _WIN32
static LRESULT CALLBACK XWindow_WndProc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    XWindow* self=nullptr;
    if(m==WM_NCCREATE){
        auto cs=reinterpret_cast<CREATESTRUCTA*>(lp);
        self=reinterpret_cast<XWindow*>(cs->lpCreateParams);
        SetWindowLongPtrA(w,GWLP_USERDATA,(LONG_PTR)self);
    } else {
        self=reinterpret_cast<XWindow*>(GetWindowLongPtrA(w,GWLP_USERDATA));
    }
    switch(m){
        case WM_ERASEBKGND:{
            HDC hdc=(HDC)wp; RECT rc;GetClientRect(w,&rc);
            HBRUSH br=self?self->bgBrush:CreateSolidBrush(RGB(32,32,32));
            FillRect(hdc,&rc,br);
            if(!self) DeleteObject(br);
            return 1;
        }
        case WM_ENTERSIZEMOVE: if(self) self->inSizeMove=true; break;
        case WM_EXITSIZEMOVE:
            if(self){
                self->inSizeMove=false;
                InvalidateRect(w,NULL,TRUE);
                UpdateWindow(w);
            }
            break;
        // case WM_SHOWWINDOW:
        //     if(self&&wp&&!self->openedFired){
        //         self->openedFired=true;
        //         triggerEvent(self->handle,"Opening",NULL);
        //     }
        //     break;
        case WM_SHOWWINDOW:
            if (self && wp && !self->openedFired)
            {
                self->openedFired = true;              // only once
                /*  Post a custom message so the event fires AFTER the window is
                    visible and the message queue returns to idle.                 */
                PostMessageW(w, WM_XWINDOW_OPEN, 0, 0);
            }
            break;

        case WM_XWINDOW_OPEN:
            if (self) triggerEvent(self->handle, "Opening", nullptr);
            return 0;          // already handled – don’t fall through

        case WM_SIZE: case WM_SIZING: case WM_MOVE: case WM_WINDOWPOSCHANGED:
            if(self&&!self->inSizeMove){
                InvalidateRect(w,NULL,TRUE);
                UpdateWindow(w);
            }
            break;
        case WM_CLOSE: case WM_DESTROY:
            if(self) triggerEvent(self->handle,"Closing",NULL);
            DestroyWindow(w);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(w,m,wp,lp);
}
#endif

extern "C"{

XPLUGIN_API int Constructor(){
    int h;
    {
        std::lock_guard<std::mutex> lk(g_instancesMtx);
        do{h=g_handleDist(g_rng);}while(g_instances.count(h));
        g_instances[h]=new XWindow(h);
    }
    return h;
}
XPLUGIN_API void Close(int h){
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        delete it->second;
        g_instances.erase(it);
    }
}

#ifdef _WIN32
XPLUGIN_API HWND XWindow_GetHWND(int h){
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        return it->second->hwnd;
    return NULL;
}
#endif

XPLUGIN_API int XWindow_Handle_GET(int h){ return h; }
XPLUGIN_API const char* Opening_GET(int h){ return strdup(("XWindow:"+std::to_string(h)+":Opening").c_str()); }
XPLUGIN_API const char* Closing_GET(int h){ return strdup(("XWindow:"+std::to_string(h)+":Closing").c_str()); }

XPLUGIN_API bool XWindow_SetEventCallback(int h,const char* n,void* cb){
    {
        std::lock_guard<std::mutex> lk(g_instancesMtx);
        if(!g_instances.count(h)) return false;
    }
    std::string key(n?n:"");
    if(auto p=key.rfind(':');p!=std::string::npos) key.erase(0,p+1);
    std::lock_guard<std::mutex> lk(g_eventMtx);
    g_eventCallbacks[h][key]=cb;
    return true;
}

//-- HasCloseButton
XPLUGIN_API void XWindow_HasCloseButton_SET(int h, bool v) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end()) return;
    it->second->HasCloseButton = v;
    it->second->updateWindowStyles();
#endif
}
XPLUGIN_API bool XWindow_HasCloseButton_GET(int h) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    return (it != g_instances.end()) ? it->second->HasCloseButton : false;
}

//-- HasMinimizeButton
XPLUGIN_API void XWindow_HasMinimizeButton_SET(int h, bool v) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end()) return;
    it->second->HasMinimizeButton = v;
    it->second->updateWindowStyles();
#endif
}
XPLUGIN_API bool XWindow_HasMinimizeButton_GET(int h) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    return (it != g_instances.end()) ? it->second->HasMinimizeButton : false;
}

//-- HasMaximizeButton
XPLUGIN_API void XWindow_HasMaximizeButton_SET(int h, bool v) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end()) return;
    it->second->HasMaximizeButton = v;
    it->second->updateWindowStyles();
#endif
}
XPLUGIN_API bool XWindow_HasMaximizeButton_GET(int h) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    return (it != g_instances.end()) ? it->second->HasMaximizeButton : false;
}

//-- HasFullScreenButton
XPLUGIN_API void XWindow_HasFullScreenButton_SET(int h, bool v) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        it->second->HasFullScreenButton = v;
#ifdef _WIN32
        it->second->updateWindowStyles();
#endif
    }
}
XPLUGIN_API bool XWindow_HasFullScreenButton_GET(int h) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    return (it != g_instances.end()) ? it->second->HasFullScreenButton : false;
}

//-- HasTitleBar
XPLUGIN_API void XWindow_HasTitleBar_SET(int h, bool v) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end()) return;
    it->second->HasTitleBar = v;
    it->second->updateWindowStyles();
#endif
}
XPLUGIN_API bool XWindow_HasTitleBar_GET(int h) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    return (it != g_instances.end()) ? it->second->HasTitleBar : false;
}

//-- Resizable
XPLUGIN_API void XWindow_Resizable_SET(int h, bool v) {
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end()) return;
    it->second->Resizable = v;
    it->second->updateWindowStyles();
#endif
}
XPLUGIN_API bool XWindow_Resizable_GET(int h) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    return (it != g_instances.end()) ? it->second->Resizable : false;
}

// Explicit Top/Left/Width/Height implementations:
XPLUGIN_API void XWindow_Top_SET(int h,int v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        MoveWindow(it->second->hwnd,rc.left,v,rc.right-rc.left,rc.bottom-rc.top,TRUE);
    }
#endif
}
XPLUGIN_API int XWindow_Top_GET(int h){
    int r=0;
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        r=rc.top;
    }
#endif
    return r;
}

XPLUGIN_API void XWindow_Left_SET(int h,int v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        MoveWindow(it->second->hwnd,v,rc.top,rc.right-rc.left,rc.bottom-rc.top,TRUE);
    }
#endif
}
XPLUGIN_API int XWindow_Left_GET(int h){
    int r=0;
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        r=rc.left;
    }
#endif
    return r;
}

XPLUGIN_API void XWindow_Width_SET(int h,int v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        MoveWindow(it->second->hwnd,rc.left,rc.top,v,rc.bottom-rc.top,TRUE);
    }
#endif
}
XPLUGIN_API int XWindow_Width_GET(int h){
    int r=0;
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        r=rc.right-rc.left;
    }
#endif
    return r;
}

XPLUGIN_API void XWindow_Height_SET(int h,int v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        MoveWindow(it->second->hwnd,rc.left,rc.top,rc.right-rc.left,v,TRUE);
    }
#endif
}
XPLUGIN_API int XWindow_Height_GET(int h){
    int r=0;
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        RECT rc; GetWindowRect(it->second->hwnd,&rc);
        r=rc.bottom-rc.top;
    }
#endif
    return r;
}

XPLUGIN_API void XWindow_Title_SET(int h,const char* v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        SetWindowTextA(it->second->hwnd,v?v:"");
#endif
}
XPLUGIN_API const char* XWindow_Title_GET(int h){
    static char buf[512]; buf[0]='\0';
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        GetWindowTextA(it->second->hwnd,buf,sizeof(buf));
#endif
    return strdup(buf);
}

XPLUGIN_API void XWindow_Enabled_SET(int h,bool v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        EnableWindow(it->second->hwnd,v);
#endif
}
XPLUGIN_API bool XWindow_Enabled_GET(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        return IsWindowEnabled(it->second->hwnd)!=0;
#endif
    return false;
}

XPLUGIN_API void XWindow_Visible_SET(int h,bool v){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        ShowWindow(it->second->hwnd,v?SW_SHOW:SW_HIDE);
#endif
}
XPLUGIN_API bool XWindow_Visible_GET(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        return IsWindowVisible(it->second->hwnd)!=0;
#endif
    return false;
}

XPLUGIN_API void XWindow_Type_SET(int h,int v){
#ifdef _WIN32
    if(v<0||v>5) return;
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        it->second->winType=v;
        XWindow::applyStyle(it->second->hwnd,v);
    }
#endif
}
XPLUGIN_API int XWindow_Type_GET(int h){
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        return it->second->winType;
    return 0;
}

XPLUGIN_API void XWindow_BackgroundColor_SET(int h,unsigned int c){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        it->second->setBackgroundColor(c);
#endif
}
XPLUGIN_API unsigned int XWindow_BackgroundColor_GET(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        return it->second->getBackgroundColor();
#endif
    return 0x202020;
}

XPLUGIN_API void XWindow_Minimize(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        ShowWindow(it->second->hwnd,SW_MINIMIZE);
#endif
}
XPLUGIN_API void XWindow_Maximize(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        ShowWindow(it->second->hwnd,SW_MAXIMIZE);
#endif
}
XPLUGIN_API void XWindow_Show(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end()){
        ShowWindow(it->second->hwnd,SW_SHOWNORMAL);
        UpdateWindow(it->second->hwnd);
    }
#endif
}
XPLUGIN_API void XWindow_Hide(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    if(auto it=g_instances.find(h);it!=g_instances.end())
        ShowWindow(it->second->hwnd,SW_HIDE);
#endif
}

//------------------------------------------------------------------------------  
// SetIcon: load a PNG from disk and apply it to the window  
//------------------------------------------------------------------------------
#ifdef _WIN32
XPLUGIN_API void XWindow_SetIcon(int h, const char* utf8Path) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end() || !it->second->hwnd) return;

    std::wstring wpath;
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8Path, -1, nullptr, 0);
        if (len > 0) {
            wpath.resize(len);
            MultiByteToWideChar(CP_UTF8, 0, utf8Path, -1, &wpath[0], len);
        }
    }
    Bitmap* bmp = Bitmap::FromFile(wpath.c_str());
    if (!bmp || bmp->GetLastStatus() != Ok) {
        delete bmp;
        return;
    }

    HICON hIcon = nullptr;
    if (bmp->GetHICON(&hIcon) == Ok && hIcon) {
        HWND wnd = it->second->hwnd;
        SendMessageW(wnd, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);
        SendMessageW(wnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
    delete bmp;
}
#endif

#ifdef _WIN32
//------------------------------------------------------------------------------
// ShowModal: disable parent, show this window, run a modal loop, then re-enable parent
//------------------------------------------------------------------------------
XPLUGIN_API void XWindow_ShowModal(int h, int parentH) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);

    auto itChild  = g_instances.find(h);
    auto itParent = g_instances.find(parentH);
    if (itChild == g_instances.end() || itParent == g_instances.end()) return;

    HWND hwndChild  = itChild->second->hwnd;
    HWND hwndParent = itParent->second->hwnd;
    if (!hwndChild || !hwndParent) return;

    EnableWindow(hwndParent, FALSE);

    ShowWindow(hwndChild, SW_SHOW);
    UpdateWindow(hwndChild);
    SetForegroundWindow(hwndChild);

    MSG msg;
    while (IsWindow(hwndChild) && IsWindowVisible(hwndChild)) {
        if (!GetMessageA(&msg, nullptr, 0, 0)) break;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    EnableWindow(hwndParent, TRUE);
    SetForegroundWindow(hwndParent);
}

//------------------------------------------------------------------------------
// MessageBox: display a modal message box disabling parent until dismissed
//------------------------------------------------------------------------------
XPLUGIN_API void XWindow_MessageBox(int h, const char* text) {
    std::lock_guard<std::mutex> lk(g_instancesMtx);
    auto it = g_instances.find(h);
    if (it == g_instances.end() || !it->second->hwnd) return;
    MessageBoxA(it->second->hwnd, text ? text : "", "Message", MB_OK | MB_ICONINFORMATION);
}
#endif

//──────────────────────────────────────── Class definition ───────────────────
typedef struct { const char* n; const char* t; void* g; void* s; } ClassProperty;
typedef struct { const char* n; void* f; int a; const char* p[10]; const char* r; } ClassEntry;
typedef struct { const char* d; } ClassConstant;
typedef struct
{
    const char*     className;
    size_t          classSize;
    void*           ctor;
    ClassProperty*  props;
    size_t          propCount;
    ClassEntry*     methods;
    size_t          methCount;
    ClassConstant*  constants;
    size_t          constCount;
} ClassDefinition;

// properties
static ClassProperty props[] =
{
    { "Handle",             "integer", (void*)XWindow_Handle_GET,              nullptr },
    { "Opening",            "string",  (void*)Opening_GET,                     nullptr },
    { "Closing",            "string",  (void*)Closing_GET,                     nullptr },
    { "Top",                "integer", (void*)XWindow_Top_GET,                 (void*)XWindow_Top_SET },
    { "Left",               "integer", (void*)XWindow_Left_GET,                (void*)XWindow_Left_SET },
    { "Width",              "integer", (void*)XWindow_Width_GET,               (void*)XWindow_Width_SET },
    { "Height",             "integer", (void*)XWindow_Height_GET,              (void*)XWindow_Height_SET },
    { "Title",              "string",  (void*)XWindow_Title_GET,               (void*)XWindow_Title_SET },
    { "Enabled",            "boolean", (void*)XWindow_Enabled_GET,             (void*)XWindow_Enabled_SET },
    { "Visible",            "boolean", (void*)XWindow_Visible_GET,             (void*)XWindow_Visible_SET },
    { "ViewType",           "integer", (void*)XWindow_Type_GET,                (void*)XWindow_Type_SET },
    { "BackgroundColor",    "color",   (void*)XWindow_BackgroundColor_GET,     (void*)XWindow_BackgroundColor_SET },
    { "HasCloseButton",     "boolean", (void*)XWindow_HasCloseButton_GET,      (void*)XWindow_HasCloseButton_SET },
    { "HasMinimizeButton",  "boolean", (void*)XWindow_HasMinimizeButton_GET,   (void*)XWindow_HasMinimizeButton_SET },
    { "HasMaximizeButton",  "boolean", (void*)XWindow_HasMaximizeButton_GET,   (void*)XWindow_HasMaximizeButton_SET },
    { "HasFullScreenButton","boolean", (void*)XWindow_HasFullScreenButton_GET, (void*)XWindow_HasFullScreenButton_SET },
    { "HasTitleBar",        "boolean", (void*)XWindow_HasTitleBar_GET,         (void*)XWindow_HasTitleBar_SET },
    { "Resizable",          "boolean", (void*)XWindow_Resizable_GET,           (void*)XWindow_Resizable_SET }
};

// methods
static ClassEntry methods[] =
{
    { "XWindow_SetEventCallback", (void*)XWindow_SetEventCallback, 3, { "integer","string","pointer" }, "boolean" },
    { "Minimize",                 (void*)XWindow_Minimize,         1, { "integer" },                    "void" },
    { "Maximize",                 (void*)XWindow_Maximize,         1, { "integer" },                    "void" },
    { "Show",                     (void*)XWindow_Show,             1, { "integer" },                    "void" },
    { "ShowModal",                (void*)XWindow_ShowModal,        2, { "integer","integer" },          "void" },
    { "Hide",                     (void*)XWindow_Hide,             1, { "integer" },                    "void" },
    { "Close",                    (void*)Close,                    1, { "integer" },                    "void" },
    { "SetIcon",                  (void*)XWindow_SetIcon,          2, { "integer","string" },           "void" },
    { "MessageBox",               (void*)XWindow_MessageBox,       2, { "integer","string" },           "void" }
};

// final class definition
static ClassDefinition classDef =
{
    "XWindow",
    sizeof(XWindow),
    (void*)Constructor,
    props,   sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() { return &classDef; }

} // extern "C"

//==============================================================================
//  Unload hook
//==============================================================================
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD why, LPVOID)
{
   if (why == DLL_PROCESS_ATTACH) {
     GdiplusStartupInput gdiplusStartupInput;
     GdiplusStartup(&gdiPlusToken, &gdiplusStartupInput, nullptr);
   }
   if(why==DLL_PROCESS_DETACH) CleanupInstances();
   if (why == DLL_PROCESS_DETACH && gdiPlusToken) {
     GdiplusShutdown(gdiPlusToken);
   }
   return TRUE;
}
#else
__attribute__((destructor)) static void onUnload() { CleanupInstances(); }
#endif
