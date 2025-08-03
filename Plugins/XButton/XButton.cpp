/*

  XButton.cpp
  CrossBasic Plugin: XButton                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
#include <cstring>
#include <iostream>
#include <random>
#include <cstdio>
#include <vector>       // std::vector
#include <algorithm>    // std::find

#ifdef _WIN32
  #include <windows.h>
  #include <commctrl.h>
  #include <dwmapi.h>
  #pragma comment(lib, "dwmapi.lib")
  #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
    #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
  #endif
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup

  static COLORREF gDarkBkg      = RGB(32,32,32);
  static COLORREF gDarkText     = RGB(255,255,255);
  static COLORREF gDisabledBkg  = RGB(48,48,48);
  static COLORREF gDisabledText = RGB(128,128,128);

  static void ApplyDarkMode(HWND hwnd) {
    BOOL dark = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  }

  static std::wstring utf8_to_wstring(const char* utf8) {
    if (!utf8) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &w[0], len);
    if (!w.empty() && w.back()==L'\0') w.pop_back();
    return w;
  }
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

#define DBG_PREFIX "XButton DEBUG: "
#define DBG(msg) do { std::cout << DBG_PREFIX << msg << std::endl; } while(0)

// forward-declare our callback-invoker
static void triggerEvent(int handle, const std::string& eventName, const char* param);

class XButton {
public:
    int handle;
    int x{0}, y{0}, width{75}, height{23};
    std::string caption;
    bool HasBorder{false};
    bool Bold{false};
    bool Underline{false};
    bool Italic{false};
    bool LockTop      {false};
    bool LockLeft     {false};
    bool LockBottom   {false};
    bool LockRight    {false};

    int  rightOffset  {0};     // distance from parent’s right edge
    int  bottomOffset {0};     // distance from parent’s bottom edge
    int parentHandle{0};

#ifdef _WIN32
    HWND hwnd{nullptr};
    bool created{false};
#endif

    explicit XButton(int h) : handle(h) {
        //DBG("Constructor: handle="<<h);
    }
    ~XButton() {
        //DBG("Destructor: handle="<<handle);
#ifdef _WIN32
        if (created && hwnd) DestroyWindow(hwnd);
#endif
    }
};

static std::mutex gInstancesMx;
static std::unordered_map<int, XButton*> gInstances;
static std::mt19937 rng(std::random_device{}());
static std::uniform_int_distribution<int> dist(10000000, 99999999);

static std::mutex gEventsMx;
static std::unordered_map<int, std::unordered_map<std::string, void*>> gEvents;

static void CleanupInstances() {
    {
        std::lock_guard<std::mutex> lk(gInstancesMx);
        for (auto& kv : gInstances) delete kv.second;
        gInstances.clear();
    }
    {
        std::lock_guard<std::mutex> lk(gEventsMx);
        gEvents.clear();
    }
}

#ifdef _WIN32
struct ButtonState { bool hover=false, pressed=false, focused=false; };
static std::unordered_map<HWND,ButtonState> gBtnState;
static std::atomic<int> gNextCtrlId{1000};

static void StartTrackingHover(HWND hWnd) {
    TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hWnd, 0 };
    TrackMouseEvent(&tme);
}

static HWND GetParentHWND(int parentHandle) {
    using Fn = HWND(*)(int);
    static Fn fn=nullptr;
    if (!fn) {
      HMODULE m=GetModuleHandleA("XWindow.dll");
      if (m) fn=(Fn)GetProcAddress(m,"XWindow_GetHWND");
    }
    return fn?fn(parentHandle):nullptr;
}

static void ReapplyFont(int h) {
    auto it = gInstances.find(h);
    if (it==gInstances.end() || !it->second->created) return;
    HWND w = it->second->hwnd;
    HFONT oldF = (HFONT)SendMessageW(w, WM_GETFONT, 0, 0);
    LOGFONTA lf{};
    if (oldF) GetObjectA(oldF, sizeof(lf), &lf);
    lf.lfWeight    = it->second->Bold      ? FW_BOLD   : FW_NORMAL;
    lf.lfItalic    = it->second->Italic    ? 1         : 0;
    lf.lfUnderline = it->second->Underline ? 1         : 0;
    HFONT f = CreateFontIndirectA(&lf);
    SendMessageW(w, WM_SETFONT, (WPARAM)f, MAKELPARAM(TRUE,0));
}

/* one entry per parent HWND that owns any “anchored” buttons */
struct AnchorSet {
    HWND                   parent {};
    std::vector<XButton*>  children;
};
static std::unordered_map<HWND,AnchorSet> gAnchors;
//static LRESULT CALLBACK ParentSizeProc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);

#ifdef _WIN32
static LRESULT CALLBACK ParentSizeProc(HWND hWnd, UINT msg,
                                       WPARAM wp, LPARAM lp,
                                       UINT_PTR idSub, DWORD_PTR ref)
{
    if (msg == WM_SIZE)
    {
        RECT prc; 
        GetClientRect(hWnd, &prc);

        auto it = gAnchors.find(hWnd);
        if (it != gAnchors.end())
        {
            for (auto *btn : it->second.children)
            {
                if (!btn->created) continue;

                // start with original position & size
                int nx = btn->x,
                    ny = btn->y,
                    nw = btn->width,
                    nh = btn->height;

                // ── HORIZONTAL ───────────────────────────────────────
                if (btn->LockLeft && btn->LockRight) {
                    // stretch width between left (btn->x) and rightOffset
                    nw = prc.right - btn->rightOffset - btn->x;
                }
                else if (btn->LockRight) {
                    // pin right edge
                    nx = prc.right - btn->rightOffset - btn->width;
                }
                // LockLeft alone just keeps nx = btn->x

                // ── VERTICAL ─────────────────────────────────────────
                if (btn->LockTop && btn->LockBottom) {
                    // stretch height between top (btn->y) and bottomOffset
                    nh = prc.bottom - btn->bottomOffset - btn->y;
                }
                else if (btn->LockBottom) {
                    // pin bottom edge
                    ny = prc.bottom - btn->bottomOffset - btn->height;
                }
                // LockTop alone just keeps ny = btn->y

                MoveWindow(btn->hwnd, nx, ny, nw, nh, TRUE);
                btn->x = nx;  btn->y = ny;
                btn->width  = nw;  btn->height = nh;

            }
        }
    }
    return DefSubclassProc(hWnd, msg, wp, lp);
}
#endif




static LRESULT CALLBACK SubclassProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                     UINT_PTR idSubclass, DWORD_PTR refData)
{
    int h = (int)refData;
    auto &st = gBtnState[hWnd];

    switch(msg) {
      case WM_CREATE:
        ApplyDarkMode(hWnd);
        SendMessageW(hWnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), MAKELPARAM(TRUE,0));
        break;
      case WM_MOUSEMOVE:
        if (!st.hover) { st.hover=true; InvalidateRect(hWnd,nullptr,TRUE); StartTrackingHover(hWnd); }
        break;
      case WM_MOUSELEAVE:
        st.hover=st.pressed=false; InvalidateRect(hWnd,nullptr,TRUE);
        break;
      case WM_LBUTTONDOWN:
        st.pressed=true; SetCapture(hWnd); InvalidateRect(hWnd,nullptr,TRUE);
        break;
      case WM_LBUTTONUP:
        if (GetCapture()==hWnd) ReleaseCapture();
        if (st.pressed) {
            st.pressed=false;
            InvalidateRect(hWnd,nullptr,TRUE);
            // >>> Pass the button handle to the callback so the script receives it as XButton
            {
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%d", h);
                triggerEvent(h, "pressed", buf);
            }
        }
        break;
      case WM_SETFOCUS:   st.focused=true;  InvalidateRect(hWnd,nullptr,TRUE); break;
      case WM_KILLFOCUS:  st.focused=false; InvalidateRect(hWnd,nullptr,TRUE); break;
      case WM_ERASEBKGND: return 1;
      case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc=BeginPaint(hWnd,&ps);
        RECT rc; GetClientRect(hWnd,&rc);
        bool enabled = IsWindowEnabled(hWnd)!=0;
        COLORREF bg = enabled
          ? ( st.pressed ? RGB(96,96,96)
                         : st.hover   ? RGB(80,80,80)
                                      : RGB(64,64,64) )
          : gDisabledBkg;
        COLORREF textColor = enabled?gDarkText:gDisabledText;
        HBRUSH br=CreateSolidBrush(bg); FillRect(dc,&rc,br); DeleteObject(br);

        { std::lock_guard<std::mutex> lk(gInstancesMx);
          auto it=gInstances.find(h);
          if(it!=gInstances.end() && it->second->HasBorder)
            FrameRect(dc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
        }

        if(st.focused) {
          RECT r2=rc; InflateRect(&r2,-1,-1);
          FrameRect(dc,&r2,(HBRUSH)GetStockObject(WHITE_BRUSH));
        }

        HFONT hf=(HFONT)SendMessageW(hWnd,WM_GETFONT,0,0);
        HGDIOBJ oldF = hf?SelectObject(dc,hf):nullptr;

        SetTextColor(dc,textColor);
        SetBkMode(dc,TRANSPARENT);
        wchar_t buf[256]={0}; GetWindowTextW(hWnd,buf,256);
        SIZE sz; GetTextExtentPoint32W(dc,buf,(int)wcslen(buf),&sz);
        int tx=(rc.right-rc.left-sz.cx)/2, ty=(rc.bottom-rc.top-sz.cy)/2;
        TextOutW(dc,rc.left+tx,rc.top+ty,buf,(int)wcslen(buf));

        if(oldF) SelectObject(dc,oldF);
        EndPaint(hWnd,&ps);
        return 0;
      }
    }
    return DefSubclassProc(hWnd,msg,wParam,lParam);
}
#endif // _WIN32

static void triggerEvent(int h, const std::string& ev, const char* param) {
    void* cb=nullptr;
    {
      std::lock_guard<std::mutex> lk(gEventsMx);
      auto it=gEvents.find(h);
      if(it!=gEvents.end()){
        auto jt=it->second.find(ev);
        if(jt!=it->second.end()) cb=jt->second;
      }
    }
    if(!cb) return;
    char* data=strdup(param?param:"");
#ifdef _WIN32
    using CB=void(__stdcall*)(const char*);
#else
    using CB=void(*)(const char*);
#endif
    ((CB)cb)(data);
    free(data);
}

extern "C" {

//------------------------------------------------------------------------------
// Constructor / Close
//------------------------------------------------------------------------------
XPLUGIN_API int Constructor(){
    int h;
    { std::lock_guard<std::mutex> lk(gInstancesMx);
      do{h=dist(rng);}while(gInstances.count(h));
      gInstances[h]=new XButton(h);
    }
    return h;
}

XPLUGIN_API void Close(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()){
      delete it->second;
      gInstances.erase(it);
    }
}

//------------------------------------------------------------------------------
// Geometry
//------------------------------------------------------------------------------
#define PROP_IMPL(NAME,member) \
XPLUGIN_API void XButton_##NAME##_SET(int h,int v){ \
  std::lock_guard<std::mutex> lk(gInstancesMx); \
  auto it=gInstances.find(h); \
  if(it!=gInstances.end()){it->second->member=v; if(it->second->created) MoveWindow(it->second->hwnd,it->second->x,it->second->y,it->second->width,it->second->height,TRUE);} \
} \
XPLUGIN_API int XButton_##NAME##_GET(int h){ \
  std::lock_guard<std::mutex> lk(gInstancesMx); \
  auto it=gInstances.find(h); \
  return it!=gInstances.end()?it->second->member:0; \
}

PROP_IMPL(X,x)
PROP_IMPL(Y,y)
PROP_IMPL(Width,width)
PROP_IMPL(Height,height)

#define BOOL_PROP(NAME,member)                                             \
XPLUGIN_API void XButton_##NAME##_SET(int h,bool v){                       \
  std::lock_guard<std::mutex> lk(gInstancesMx);                            \
  auto it=gInstances.find(h);                                              \
  if(it==gInstances.end()) return;                                         \
  XButton* B = it->second;                                                 \
  bool old = B->member;                                                    \
  B->member = v;                                                           \
  /* If flag changed **after** the control already exists, (re)register */ \
  if (B->created && v != old) {                                            \
      HWND p = GetParentHWND(B->parentHandle);                             \
      if (p) {                                                             \
          auto &aset = gAnchors[p];                                        \
          if (std::find(aset.children.begin(), aset.children.end(), B) ==  \
              aset.children.end())                                         \
          {                                                                \
              /* first-time registration for this button */                \
              RECT prc, crc;                                               \
              GetClientRect(p,&prc); GetWindowRect(B->hwnd,&crc);          \
              MapWindowPoints(HWND_DESKTOP,p,(POINT*)&crc,2);              \
              B->rightOffset  = prc.right  - crc.right;                    \
              B->bottomOffset = prc.bottom - crc.bottom;                   \
              aset.parent = p;                                             \
              aset.children.push_back(B);                                  \
              if (aset.children.size()==1)                                 \
                  SetWindowSubclass(p, ParentSizeProc, 0xFEED, 0);         \
          }                                                                \
      }                                                                    \
  }                                                                        \
}                                                                           \
XPLUGIN_API bool XButton_##NAME##_GET(int h){                              \
  std::lock_guard<std::mutex> lk(gInstancesMx);                            \
  auto it=gInstances.find(h);                                              \
  return it!=gInstances.end()?it->second->member:false;                    \
}


BOOL_PROP(LockTop,    LockTop)
BOOL_PROP(LockLeft,   LockLeft)
BOOL_PROP(LockRight,  LockRight)
BOOL_PROP(LockBottom, LockBottom)


//------------------------------------------------------------------------------
// Parent
//------------------------------------------------------------------------------
// XPLUGIN_API void XButton_Parent_SET(int h,int ph){
//     std::lock_guard<std::mutex> lk(gInstancesMx);
//     auto it=gInstances.find(h);
//     if(it==gInstances.end()) return;
//     it->second->parentHandle=ph;
// #ifdef _WIN32
//     if(it->second->created) DestroyWindow(it->second->hwnd);
//     HWND p=GetParentHWND(ph);
//     if(!p) return;
//     auto& B=*it->second;
//     int id=gNextCtrlId++;
//     auto wcap=utf8_to_wstring(B.caption.c_str());
//     B.hwnd=CreateWindowExW(0,L"BUTTON",wcap.c_str(),
//       WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
//       B.x,B.y,B.width,B.height,
//       p,(HMENU)(intptr_t)id,GetModuleHandleW(NULL),NULL);
//     if(!B.hwnd) return;
//     B.created=true;
//     ApplyDarkMode(B.hwnd);
//     SetWindowSubclass(B.hwnd,SubclassProc,id,(DWORD_PTR)h);
// #endif
// }
XPLUGIN_API void XButton_Parent_SET(int h,int ph)
{
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it==gInstances.end()) return;
    XButton& B=*it->second;
    B.parentHandle=ph;

#ifdef _WIN32
    /* destroy any previous HWND so we recreate inside the new parent */
    if (B.created && B.hwnd) {
        DestroyWindow(B.hwnd);
        B.created=false;
    }

    HWND p=GetParentHWND(ph);
    if(!p) return;

    /* -------------------------------- create the control ---------------- */
    int id=gNextCtrlId++;
    auto wcap=utf8_to_wstring(B.caption.c_str());
    B.hwnd=CreateWindowExW(0,L"BUTTON",wcap.c_str(),
        WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,
        B.x,B.y,B.width,B.height,
        p,(HMENU)(intptr_t)id,GetModuleHandleW(NULL),NULL);
    if(!B.hwnd) return;

    B.created=true;
    ApplyDarkMode(B.hwnd);
    SetWindowSubclass(B.hwnd,SubclassProc,id,(DWORD_PTR)h);

    /* --------- anchor bookkeeping (only if any lock flag is true) -------- */
    bool anchored = B.LockTop||B.LockLeft||B.LockBottom||B.LockRight;
    if (anchored)
    {
        RECT prc, crc; GetClientRect(p,&prc); GetWindowRect(B.hwnd,&crc);
        MapWindowPoints(HWND_DESKTOP,p,(POINT*)&crc,2);

        if (B.LockRight)  B.rightOffset  = prc.right  - crc.right;
        if (B.LockBottom) B.bottomOffset = prc.bottom - crc.bottom;

        auto &aset = gAnchors[p];               // creates entry if absent
        aset.parent = p;
        aset.children.push_back(&B);

        /* first time we touch this parent → subclass it */
        if (aset.children.size()==1)
            SetWindowSubclass(p, ParentSizeProc, 0xFEED, 0);
    }
#endif
}




XPLUGIN_API int XButton_Parent_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    return it!=gInstances.end()?it->second->parentHandle:0;
}

//------------------------------------------------------------------------------
// Caption
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_Caption_SET(int h,const char* utf8){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it==gInstances.end()||!it->second->created) return;
    it->second->caption=utf8?utf8:"";
#ifdef _WIN32
    auto wcap=utf8_to_wstring(it->second->caption.c_str());
    SetWindowTextW(it->second->hwnd,wcap.c_str());
#else
    SetWindowTextA(it->second->hwnd,it->second->caption.c_str());
#endif

    InvalidateRect(it->second->hwnd, nullptr, TRUE);
}
XPLUGIN_API const char* XButton_Caption_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    return it!=gInstances.end()?strdup(it->second->caption.c_str()):strdup("");
}

//------------------------------------------------------------------------------
// HasBorder
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_HasBorder_SET(int h,bool b){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()){
      it->second->HasBorder=b;
      if(it->second->created) InvalidateRect(it->second->hwnd,nullptr,TRUE);
    }
}
XPLUGIN_API bool XButton_HasBorder_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    return it!=gInstances.end()?it->second->HasBorder:false;
}

//------------------------------------------------------------------------------
// Bold
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_Bold_SET(int h,bool b){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()){
      it->second->Bold=b;
      ReapplyFont(h);
    }
}
XPLUGIN_API bool XButton_Bold_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    return it!=gInstances.end()?it->second->Bold:false;
}

//------------------------------------------------------------------------------
// Underline
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_Underline_SET(int h,bool u){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()){
      it->second->Underline=u;
      ReapplyFont(h);
    }
}
XPLUGIN_API bool XButton_Underline_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    return it!=gInstances.end()?it->second->Underline:false;
}

//------------------------------------------------------------------------------
// Italic
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_Italic_SET(int h,bool i){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()){
      it->second->Italic=i;
      ReapplyFont(h);
    }
}
XPLUGIN_API bool XButton_Italic_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    return it!=gInstances.end()?it->second->Italic:false;
}

//------------------------------------------------------------------------------
// Pressed token
//------------------------------------------------------------------------------
XPLUGIN_API const char* Pressed_GET(int h){
    std::string s="XButton:"+std::to_string(h)+":pressed";
    return strdup(s.c_str());
}

//------------------------------------------------------------------------------
// Event hookup
//------------------------------------------------------------------------------
XPLUGIN_API bool XButton_SetEventCallback(int h,const char* ev,void* cb){
    {
      std::lock_guard<std::mutex> lk(gInstancesMx);
      if(!gInstances.count(h)) return false;
    }
    std::string key=ev?ev:"";
    if(auto p=key.rfind(':');p!=std::string::npos) key.erase(0,p+1);
    std::lock_guard<std::mutex> lk2(gEventsMx);
    gEvents[h][key]=cb;
    return true;
}

//------------------------------------------------------------------------------
// FontName / FontSize
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_FontName_SET(int h,const char* name){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      HWND w=it->second->hwnd;
      HFONT oldF=(HFONT)SendMessageA(w,WM_GETFONT,0,0);
      if(!oldF) oldF=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
      LOGFONTA lf{}; GetObjectA(oldF,sizeof(lf),&lf);
      strncpy_s(lf.lfFaceName,LF_FACESIZE,name?name:lf.lfFaceName,_TRUNCATE);
      HFONT f=CreateFontIndirectA(&lf);
      SendMessageW(w,WM_SETFONT,(WPARAM)f,MAKELPARAM(TRUE,0));
      ReapplyFont(h);
#endif
    }
}
XPLUGIN_API const char* XButton_FontName_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      HWND w=it->second->hwnd;
      HFONT oldF=(HFONT)SendMessageA(w,WM_GETFONT,0,0);
      if(!oldF) oldF=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
      LOGFONTA lf{}; GetObjectA(oldF,sizeof(lf),&lf);
      return strdup(lf.lfFaceName);
#endif
    }
    return strdup("");
}
XPLUGIN_API void XButton_FontSize_SET(int h,int size){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      HWND w=it->second->hwnd;
      HFONT oldF=(HFONT)SendMessageA(w,WM_GETFONT,0,0);
      if(!oldF) oldF=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
      LOGFONTA lf{}; GetObjectA(oldF,sizeof(lf),&lf);
      lf.lfHeight=-size;
      HFONT f=CreateFontIndirectA(&lf);
      SendMessageW(w,WM_SETFONT,(WPARAM)f,MAKELPARAM(TRUE,0));
      ReapplyFont(h);
#endif
    }
}
XPLUGIN_API int XButton_FontSize_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      HWND w=it->second->hwnd;
      HFONT oldF=(HFONT)SendMessageA(w,WM_GETFONT,0,0);
      if(!oldF) oldF=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
      LOGFONTA lf{}; GetObjectA(oldF,sizeof(lf),&lf);
      return -lf.lfHeight;
#endif
    }
    return 0;
}

//------------------------------------------------------------------------------
// Enabled / Visible
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_Enabled_SET(int h,bool e){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      EnableWindow(it->second->hwnd,e);

      // mark the entire button rect dirty so WM_PAINT will run
      InvalidateRect(it->second->hwnd, nullptr, TRUE);
      // optional: force immediate repaint
      // UpdateWindow(it->second->hwnd);
#endif
    }
}
XPLUGIN_API bool XButton_Enabled_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      return IsWindowEnabled(it->second->hwnd)!=0;
#else
      return false;
#endif
    }
    return false;
}
XPLUGIN_API void XButton_Visible_SET(int h,bool v){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      ShowWindow(it->second->hwnd, v?SW_SHOW:SW_HIDE);
#endif
    }
}
XPLUGIN_API bool XButton_Visible_GET(int h){
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it=gInstances.find(h);
    if(it!=gInstances.end()&&it->second->created){
#ifdef _WIN32
      return IsWindowVisible(it->second->hwnd)!=0;
#else
      return false;
#endif
    }
    return false;
}

//------------------------------------------------------------------------------  
// Invalidate: force a repaint of the button  
//------------------------------------------------------------------------------
XPLUGIN_API void XButton_Invalidate(int h) {
    std::lock_guard<std::mutex> lk(gInstancesMx);
    auto it = gInstances.find(h);
    if (it != gInstances.end() && it->second->created) {
#ifdef _WIN32
        InvalidateRect(it->second->hwnd, nullptr, TRUE);
#endif
    }
}


//------------------------------------------------------------------------------
// ClassDefinition + DllMain
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
    { "Left","integer",(void*)XButton_X_GET,(void*)XButton_X_SET },
    { "Top","integer",(void*)XButton_Y_GET,(void*)XButton_Y_SET },
    { "Width","integer",(void*)XButton_Width_GET,(void*)XButton_Width_SET },
    { "Height","integer",(void*)XButton_Height_GET,(void*)XButton_Height_SET },
    { "Parent","integer",(void*)XButton_Parent_GET,(void*)XButton_Parent_SET },
    { "Caption","string",(void*)XButton_Caption_GET,(void*)XButton_Caption_SET },
    { "HasBorder","boolean",(void*)XButton_HasBorder_GET,(void*)XButton_HasBorder_SET },
    { "Bold","boolean",(void*)XButton_Bold_GET,(void*)XButton_Bold_SET },
    { "Underline","boolean",(void*)XButton_Underline_GET,(void*)XButton_Underline_SET },
    { "Italic","boolean",(void*)XButton_Italic_GET,(void*)XButton_Italic_SET },
    { "Pressed","string",(void*)Pressed_GET,nullptr },
    { "FontName","string",(void*)XButton_FontName_GET,(void*)XButton_FontName_SET },
    { "FontSize","integer",(void*)XButton_FontSize_GET,(void*)XButton_FontSize_SET },
    { "Enabled","boolean",(void*)XButton_Enabled_GET,(void*)XButton_Enabled_SET },
    { "Visible","boolean",(void*)XButton_Visible_GET,(void*)XButton_Visible_SET },
    { "LockTop",    "boolean", (void*)XButton_LockTop_GET,    (void*)XButton_LockTop_SET },
    { "LockLeft",   "boolean", (void*)XButton_LockLeft_GET,   (void*)XButton_LockLeft_SET },
    { "LockRight",  "boolean", (void*)XButton_LockRight_GET,  (void*)XButton_LockRight_SET },
    { "LockBottom", "boolean", (void*)XButton_LockBottom_GET, (void*)XButton_LockBottom_SET }
};

static ClassEntry methods[] = {
    { "XButton_SetEventCallback",(void*)XButton_SetEventCallback,3,{"integer","string","pointer"},"boolean" },
    { "Invalidate",(void*)XButton_Invalidate,1,{"integer"},"void" },
    { "Close",   (void*)Close,             1, {"integer"}, "void"    },
};

static ClassDefinition classDef = {
    "XButton",
    sizeof(XButton),
    (void*)Constructor,
    props, sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition(){
    return &classDef;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_DETACH) CleanupInstances();
    return TRUE;
}
#else
__attribute__((destructor))
static void onUnload(){ CleanupInstances(); }
#endif

} // extern "C"
