/*

  XListbox.cpp
  CrossBasic Plugin: XListbox                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
/* ── Build commands ──────────────────────────────────────────────────────────
   windres XListbox.rc -O coff -o XListbox.res

   g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ ^
       -o XListbox.dll XListbox.cpp XListbox.res ^
       -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -lole32
*/

#ifdef _WIN32
  #define UNICODE
  #define _UNICODE
  #ifndef _WIN32_IE
    #define _WIN32_IE 0x0500
  #endif
  #include <windows.h>
  #include <commctrl.h>
  #include <dwmapi.h>
  #pragma comment(lib,"dwmapi.lib")
  #pragma comment(lib,"comctl32.lib")
  #ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
    #define DWMWA_USE_IMMERSIVE_DARK_MODE 20
  #endif
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

#ifndef HDM_SETBKCOLOR
  #define HDM_FIRST         0x1200
  #define HDM_SETBKCOLOR    (HDM_FIRST + 14)
  #define HDM_SETTEXTCOLOR  (HDM_FIRST + 15)
#endif

#include <mutex>
#include <unordered_map>
#include <atomic>
#include <string>
#include <vector>
#include <sstream>
#include <random>
#include <cstdlib>
#include <cstring>

// ─────────────────────────────────────────────────────────────────────────────
// helpers
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
static std::wstring utf8_to_w(const char* s)
{
    if(!s) return L"";
    int len=MultiByteToWideChar(CP_UTF8,0,s,-1,nullptr,0);
    if(len<=0) return L"";
    std::wstring w(len,0);
    MultiByteToWideChar(CP_UTF8,0,s,-1,&w[0],len);
    w.pop_back();
    return w;
}
static std::string w_to_utf8(const wchar_t* w)
{
    if(!w) return{};
    int len=WideCharToMultiByte(CP_UTF8,0,w,-1,nullptr,0,nullptr,nullptr);
    if(len<=0) return{};
    std::string s(len,0);
    WideCharToMultiByte(CP_UTF8,0,w,-1,&s[0],len,nullptr,nullptr);
    s.pop_back();
    return s;
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
static COLORREF gDarkBkg      = RGB(32,32,32);
static COLORREF gDarkText     = RGB(255,255,255);
static void triggerEvent(int h,const std::string& ev,const char* param); // fwd

class XListbox {
public:
    int handle;
    int x{0},y{0},width{120},height{100};
    int columnCount{0};
    std::string columnWidths;
    int RowHeight{0};
    bool Enabled{true};
    std::string FontName;
    int FontSize{0};
    bool HasHeader{true};
    std::string InitialValue;
    int LastAddedRowIndex{-1},LastColumnIndex{-1},LastRowIndex{-1};
    bool Visible{true};
    int parentHandle{0};
    bool HasBorder{false};
    COLORREF BorderColor{ RGB(255,255,255) }; // default white
    COLORREF TextColor{ gDarkText };          // default dark‑mode text


#ifdef _WIN32
    HWND hwnd{nullptr};
    bool created{false};
#endif
    explicit XListbox(int h):handle(h){}
    ~XListbox(){ if(created&&hwnd) DestroyWindow(hwnd); }
};

// ── globals ──────────────────────────────────────────────────────────────────
static std::mutex gMx;
static std::unordered_map<int,XListbox*> gObj;
static std::mt19937 rng(std::random_device{}());
static std::uniform_int_distribution<int> rnd(1'000'000,9'999'999);
static std::mutex gEvMx;
static std::unordered_map<int,std::unordered_map<std::string,void*>> gEv;

#ifdef _WIN32
static std::atomic<int> gNextId{1000};
static bool gComCtlInit=false;

static HWND GetParentHWND(int ph)
{
    using Fn = HWND(*)(int);
    static Fn fn=nullptr;
    if(!fn){
        HMODULE m=GetModuleHandleA("XWindow.dll");
        if(m) fn=(Fn)GetProcAddress(m,"XWindow_GetHWND");
    }
    return fn?fn(ph):nullptr;
}
static void ApplyDark(HWND w){ BOOL d=TRUE; DwmSetWindowAttribute(w,DWMWA_USE_IMMERSIVE_DARK_MODE,&d,sizeof(d)); }

// static LRESULT CALLBACK SubProc(HWND hWnd,UINT m,WPARAM w,LPARAM l,
//                                 UINT_PTR, DWORD_PTR ref)
// {
//     if(m==WM_NOTIFY){
//         LPNMHDR nm=(LPNMHDR)l;
//         if(nm->code==LVN_ITEMCHANGED){
//             int h=(int)ref;
//             XListbox* lb=nullptr;
//             { std::lock_guard<std::mutex>lk(gMx); auto it=gObj.find(h); if(it!=gObj.end())lb=it->second; }
//             if(lb){
//                 lb->LastRowIndex=(int)SendMessageW(hWnd,LVM_GETNEXTITEM,-1,LVNI_SELECTED);
//                 triggerEvent(h,"SelectionChanged",nullptr);
//             }
//         }
//     }

//     case WM_PAINT: {
//         // let default draw list
//         LRESULT lr = DefSubclassProc(hWnd, WM_PAINT, wParam, lParam);
    
//         // then draw our border if requested
//         int h = (int)refData;
//         XListbox* lb = nullptr;
//         { std::lock_guard<std::mutex> lk(gMx);
//           auto it = gObj.find(h);
//           if(it!=gObj.end()) lb = it->second;
//         }
//         if(lb && lb->HasBorder) {
//           RECT rc; GetClientRect(hWnd, &rc);
//           HDC dc = GetWindowDC(hWnd);
//           HPEN pen = CreatePen(PS_SOLID, 1, lb->BorderColor);
//           HGDIOBJ old = SelectObject(dc, pen);
//           // draw rectangle one pixel inside
//           Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);
//           SelectObject(dc, old);
//           DeleteObject(pen);
//           ReleaseDC(hWnd, dc);
//         }
//         return lr;
//     }
    


//     return DefSubclassProc(hWnd,m,w,l);
// }

static LRESULT CALLBACK SubProc(HWND hWnd, UINT m, WPARAM wParam, LPARAM lParam, UINT_PTR /*idSubclass*/, DWORD_PTR refData)
{
    //XListbox *lb = nullptr;
    int h = (int)refData;
    auto itGlobal = gObj.find((int)refData);
    XListbox* lb = itGlobal!=gObj.end() ? itGlobal->second : nullptr;

    switch (m) {
        

    case WM_LBUTTONDOWN: {
        if (lb && lb->created) {
          int oldSel = lb->LastRowIndex;
          // let the listview process the click (so selection changes internally)
          LRESULT r = DefSubclassProc(hWnd, m, wParam, lParam);
          int newSel = (int)SendMessageW(hWnd, LVM_GETNEXTITEM, -1, LVNI_SELECTED);
          if (newSel != oldSel) {
            lb->LastRowIndex = newSel;
            triggerEvent(lb->handle, "SelectionChanged", nullptr);
          }
          return r;
        }
        break;
      }

    case WM_NOTIFY:
    {
    auto hdr = (NMHDR*)lParam;

    // 2a) custom‐draw notification
    if (hdr->code == NM_CUSTOMDRAW)
    {
        auto cd = (NMLVCUSTOMDRAW*)lParam;
        switch (cd->nmcd.dwDrawStage)
        {
        case CDDS_PREPAINT:
            // ask for per‑item notifications
            return CDRF_NOTIFYITEMDRAW;

        case CDDS_ITEMPREPAINT:
        {
            // color each row’s text with lb->TextColor
            std::lock_guard<std::mutex> lk(gMx);
            auto it = gObj.find(h);
            if (it != gObj.end())
            {
            COLORREF txt = it->second->TextColor;
            SetTextColor(cd->nmcd.hdc, txt);
            SetBkMode(cd->nmcd.hdc, TRANSPARENT);
            }
            return CDRF_NEWFONT;
        }
        }
    }

    // 2b) selection change
    if (hdr->code == LVN_ITEMCHANGED)
    {
        auto lv = (NMLISTVIEW*)lParam;
        // only fire when an item actually became selected
        if ( (lv->uChanged & LVIF_STATE) &&
            (lv->uNewState & LVIS_SELECTED) )
        {
        std::lock_guard<std::mutex> lk(gMx);
        auto it = gObj.find(h);
        if(it!=gObj.end())
        {
            it->second->LastRowIndex = lv->iItem;
            triggerEvent(h, "SelectionChanged", nullptr);
        }
        }
    }
    break;
    }


    case WM_PAINT:
    {
        // Let the default paint run first
        LRESULT lr = DefSubclassProc(hWnd, WM_PAINT, wParam, lParam);

        // Then draw our 1px border if requested
        {
            std::lock_guard<std::mutex> lk(gMx);
            auto it = gObj.find(h);
            if (it != gObj.end())
                lb = it->second;
        }

        if (lb && lb->HasBorder)
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            HDC dc = GetWindowDC(hWnd);
            HPEN pen = CreatePen(PS_SOLID, 1, lb->BorderColor);
            HGDIOBJ old = SelectObject(dc, pen);

            // draw the rectangle
            Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom);

            // cleanup
            SelectObject(dc, old);
            DeleteObject(pen);
            ReleaseDC(hWnd, dc);
        }

        return lr;
    }

    default:
        // all other messages go to default handler
        break;
    }

    return DefSubclassProc(hWnd, m, wParam, lParam);
}

#endif

// ── utilities ────────────────────────────────────────────────────────────────
static int genHandle(){ int h; do{h=rnd(rng);}while(gObj.count(h)); return h; }

// ── core API ─────────────────────────────────────────────────────────────────
extern "C"{

XPLUGIN_API int Constructor(){
    int h=genHandle();
    std::lock_guard<std::mutex>lk(gMx);
    gObj[h]=new XListbox(h);
    return h;
}
XPLUGIN_API void Close(int h){
    std::lock_guard<std::mutex>lk(gMx);
    auto it=gObj.find(h);
    if(it!=gObj.end()){ delete it->second; gObj.erase(it); }
}

// ── simple int property macro ───────────────────────────────────────────────
#define INT_PROP(name,member)                                              \
XPLUGIN_API int  XListbox_##name##_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?it->second->member:0;}\
XPLUGIN_API void XListbox_##name##_SET(int h,int v){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end()){it->second->member=v;if(it->second->created)MoveWindow(it->second->hwnd,it->second->x,it->second->y,it->second->width,it->second->height,TRUE);} }
INT_PROP(Left,x) INT_PROP(Top,y) INT_PROP(Width,width) INT_PROP(Height,height)
INT_PROP(ColumnCount,columnCount) INT_PROP(RowHeight,RowHeight)
INT_PROP(LastAddedRowIndex,LastAddedRowIndex)
INT_PROP(LastColumnIndex,LastColumnIndex) INT_PROP(LastRowIndex,LastRowIndex)
#undef INT_PROP

// strings / misc
XPLUGIN_API void XListbox_ColumnWidths_SET(int h,const char* s){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end())it->second->columnWidths=s?s:""; }
XPLUGIN_API const char* XListbox_ColumnWidths_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?strdup(it->second->columnWidths.c_str()):strdup(""); }

XPLUGIN_API void XListbox_FontName_SET(int h,const char* nm){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end())it->second->FontName=nm?nm:""; }
XPLUGIN_API const char* XListbox_FontName_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?strdup(it->second->FontName.c_str()):strdup(""); }

XPLUGIN_API void XListbox_FontSize_SET(int h,int sz){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end())it->second->FontSize=sz; }
XPLUGIN_API int  XListbox_FontSize_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?it->second->FontSize:0; }

XPLUGIN_API void XListbox_Enabled_SET(int h,bool b){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end()&&it->second->created)EnableWindow(it->second->hwnd,b); }
XPLUGIN_API bool XListbox_Enabled_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?it->second->Enabled:false; }

XPLUGIN_API void XListbox_Visible_SET(int h,bool v){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end()&&it->second->created)ShowWindow(it->second->hwnd,v?SW_SHOW:SW_HIDE); }
XPLUGIN_API bool XListbox_Visible_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?it->second->Visible:false; }

XPLUGIN_API void XListbox_HasHeader_SET(int h,bool b){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end()){it->second->HasHeader=b;if(it->second->created){HWND hdr=(HWND)SendMessageW(it->second->hwnd,LVM_GETHEADER,0,0);if(hdr)ShowWindow(hdr,b?SW_SHOW:SW_HIDE);}}}
XPLUGIN_API bool XListbox_HasHeader_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?it->second->HasHeader:false; }

XPLUGIN_API void XListbox_InitialValue_SET(int h,const char* v){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end())it->second->InitialValue=v?v:""; }
XPLUGIN_API const char* XListbox_InitialValue_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?strdup(it->second->InitialValue.c_str()):strdup(""); }

XPLUGIN_API int XListbox_RowCount_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return(it!=gObj.end()&&it->second->created)?(int)SendMessageW(it->second->hwnd,LVM_GETITEMCOUNT,0,0):0; }

XPLUGIN_API void XListbox_SelectedRow_SET(int h, int i) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    if (it == gObj.end() || !it->second->created) return;
    XListbox& LB = *it->second;

    int oldSel = LB.LastRowIndex;

    // select new item
    LVITEMW lv{};
    lv.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
    lv.state     = LVIS_SELECTED|LVIS_FOCUSED;
    SendMessageW(LB.hwnd, LVM_SETITEMSTATE, i, (LPARAM)&lv);

    // update our internal index
    LB.LastRowIndex = i;

    // only fire if it really changed
    if (i != oldSel) {
      triggerEvent(h, "SelectionChanged", nullptr);
    }
}


//XPLUGIN_API void XListbox_SelectedRow_SET(int h,int i){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);if(it!=gObj.end()&&it->second->created){LVITEMW lv{}; lv.stateMask=LVIS_SELECTED|LVIS_FOCUSED; lv.state=LVIS_SELECTED|LVIS_FOCUSED; SendMessageW(it->second->hwnd,LVM_SETITEMSTATE,i,(LPARAM)&lv);} }
XPLUGIN_API int  XListbox_SelectedRow_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return(it!=gObj.end()&&it->second->created)?(int)SendMessageW(it->second->hwnd,LVM_GETNEXTITEM,-1,LVNI_SELECTED):-1; }




// ── Parent (creates the control) ────────────────────────────────────────────
XPLUGIN_API void XListbox_Parent_SET(int h,int ph){
    #ifdef _WIN32
        std::lock_guard<std::mutex> lk(gMx);
        auto it = gObj.find(h);
        if(it == gObj.end()) return;
    
        if(!gComCtlInit){
          INITCOMMONCONTROLSEX icc{ sizeof(icc), ICC_LISTVIEW_CLASSES };
          InitCommonControlsEx(&icc);
          gComCtlInit = true;
        }
    
        XListbox &LB = *it->second;
        if(LB.created)
          DestroyWindow(LB.hwnd);
    
        HWND parent = GetParentHWND(ph);
        if(!parent) return;
    
        int id = gNextId++;
        LB.hwnd = CreateWindowExW(
          0,
          WC_LISTVIEWW,
          L"",
          WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
          LB.x, LB.y, LB.width, LB.height,
          parent,
          (HMENU)(intptr_t)id,
          GetModuleHandleW(nullptr),
          nullptr
        );
        if(!LB.hwnd) return;
    
        LB.created      = true;
        LB.parentHandle = ph;
    
        // disable theming so our colors stick
        SetWindowTheme(LB.hwnd, L"", L"");
        HWND header = ListView_GetHeader(LB.hwnd);
        if(header)
          SetWindowTheme(header, L"", L"");
    
        // 1) full‑row & double‑buffer
        ListView_SetExtendedListViewStyle(
          LB.hwnd,
          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
        );
    
        // 2) body colors
        SendMessageW(LB.hwnd, LVM_SETBKCOLOR,      0,            (LPARAM)gDarkBkg);
        SendMessageW(LB.hwnd, LVM_SETTEXTCOLOR,   (WPARAM)LB.TextColor, 0);
        SendMessageW(LB.hwnd, LVM_SETTEXTBKCOLOR, 0,            (LPARAM)gDarkBkg);
    
        // 3) header colors
        if(header){
          SendMessageW(header, HDM_SETBKCOLOR,   0, (LPARAM)gDarkBkg);
          SendMessageW(header, HDM_SETTEXTCOLOR, 0, (LPARAM)LB.TextColor);
        }
    
        // 4) Windows‑10/11 dark scrollbars, etc.
        ApplyDark(LB.hwnd);
    
        // 5) apply font
        if(!LB.FontName.empty() || LB.FontSize){
          LOGFONTW lf{};
          lf.lfHeight = LB.FontSize? -LB.FontSize : 0;
          if(!LB.FontName.empty()){
            auto wn = utf8_to_w(LB.FontName.c_str());
            wcsncpy(lf.lfFaceName, wn.c_str(), LF_FACESIZE-1);
          }
          HFONT f = CreateFontIndirectW(&lf);
          SendMessageW(LB.hwnd, WM_SETFONT, (WPARAM)f, TRUE);
        }
    
        // 6) columns
        for(int c = 0; c < LB.columnCount; ++c){
          LVCOLUMNW col{};
          col.mask    = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
          col.pszText = (LPWSTR)(std::wstring(L"Column ") + std::to_wstring(c+1)).c_str();
          col.cx      = 80;
          col.fmt     = LVCFMT_LEFT;
          ListView_InsertColumn(LB.hwnd, c, &col);
        }
    
        // 7) initial rows
        std::istringstream ss(LB.InitialValue);
        std::string line;
        int row = 0;
        while(std::getline(ss, line, '\n')){
          auto wrow = utf8_to_w(line.c_str());
          LVITEMW item{};
          item.mask     = LVIF_TEXT;
          item.iItem    = row++;
          item.iSubItem = 0;
          item.pszText  = (LPWSTR)wrow.c_str();
          ListView_InsertItem(LB.hwnd, &item);
        }
    
        // 8) hook our SubProc
        SetWindowSubclass(LB.hwnd, SubProc, id, (DWORD_PTR)h);
    #endif
    }
// XPLUGIN_API void XListbox_Parent_SET(int h,int ph){
// #ifdef _WIN32
//     std::lock_guard<std::mutex>lk(gMx);
//     auto it=gObj.find(h); if(it==gObj.end())return;
//     if(!gComCtlInit){ INITCOMMONCONTROLSEX icc{sizeof(icc),ICC_LISTVIEW_CLASSES}; InitCommonControlsEx(&icc); gComCtlInit=true; }
//     XListbox& LB=*it->second;
//     if(LB.created) DestroyWindow(LB.hwnd);

//     HWND parent=GetParentHWND(ph); if(!parent) return;
//     int id=gNextId++;
//     LB.hwnd=CreateWindowExW(0,WC_LISTVIEWW,L"",WS_CHILD|WS_VISIBLE|LVS_REPORT|LVS_SHOWSELALWAYS,
//                             LB.x,LB.y,LB.width,LB.height,
//                             parent,(HMENU)(intptr_t)id,GetModuleHandleW(nullptr),nullptr);
//     if(!LB.hwnd) return;
//     LB.created=true; LB.parentHandle=ph;

//     SetWindowTheme(LB.hwnd, L"", L"");
//     HWND hdr = ListView_GetHeader(LB.hwnd);
//     if (hdr) SetWindowTheme(hdr, L"", L"");

//     // 1) turn on full‑row select & double‑buffering
//     ListView_SetExtendedListViewStyle(LB.hwnd,
//         LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

//     // 2) paint the LIST itself dark
//     SendMessageW(LB.hwnd, LVM_SETBKCOLOR,        0,       (LPARAM)gDarkBkg);
//     SendMessageW(LB.hwnd, LVM_SETTEXTCOLOR,    (WPARAM)gDarkText, 0);
//     SendMessageW(LB.hwnd, LVM_SETTEXTBKCOLOR,   0,       (LPARAM)gDarkBkg);

//     // 3) paint the HEADER control dark, via header messages
//     HWND hdr = ListView_GetHeader(LB.hwnd);
//     if(hdr) {
//       // HDM_SETBKCOLOR and HDM_SETTEXTCOLOR are defined in commctrl.h
//       SendMessageW(hdr, HDM_SETBKCOLOR,   0, (LPARAM)gDarkBkg);
//       SendMessageW(hdr, HDM_SETTEXTCOLOR, 0, (LPARAM)gDarkText);
//     }

//     // finally, let Windows flip us into dark mode if available
//     ApplyDark(LB.hwnd);

//     // font
//     if(!LB.FontName.empty()||LB.FontSize){
//         LOGFONTW lf{}; lf.lfHeight=LB.FontSize? -LB.FontSize:0;
//         if(!LB.FontName.empty()){
//             std::wstring wn=utf8_to_w(LB.FontName.c_str());
//             wcsncpy(lf.lfFaceName,wn.c_str(),LF_FACESIZE-1);
//         }
//         HFONT f=CreateFontIndirectW(&lf);
//         SendMessageW(LB.hwnd,WM_SETFONT,(WPARAM)f,TRUE);
//     }

//     // columns
//     for(int c=0;c<LB.columnCount;++c){
//         LVCOLUMNW col{}; col.mask=LVCF_TEXT|LVCF_WIDTH|LVCF_FMT;
//         std::wstring hdr=L"Column "+std::to_wstring(c+1);
//         col.pszText=(LPWSTR)hdr.c_str(); col.cx=80; col.fmt=LVCFMT_LEFT;
//         ListView_InsertColumn(LB.hwnd,c,&col);
//     }

//     // initial data
//     std::istringstream ss(LB.InitialValue);
//     std::string line; int row=0;
//     while(std::getline(ss,line,'\n')){
//         std::wstring wrow=utf8_to_w(line.c_str());
//         LVITEMW item{}; item.mask=LVIF_TEXT; item.iItem=row; item.iSubItem=0;
//         item.pszText=(LPWSTR)wrow.c_str();
//         ListView_InsertItem(LB.hwnd,&item);
//         ++row;
//     }

//     SetWindowSubclass(LB.hwnd,SubProc,id,(DWORD_PTR)h);
// #else
//     (void)h;(void)ph;
// #endif
// }

XPLUGIN_API int XListbox_Parent_GET(int h){ std::lock_guard<std::mutex>lk(gMx);auto it=gObj.find(h);return it!=gObj.end()?it->second->parentHandle:0; }

// ── Methods ─────────────────────────────────────────────────────────────────
XPLUGIN_API void XListbox_AddRow(int h,const char* r){
#ifdef _WIN32
    std::lock_guard<std::mutex>lk(gMx); auto it=gObj.find(h); if(it==gObj.end()||!it->second->created)return;
    XListbox& LB=*it->second;
    int idx=(int)SendMessageW(LB.hwnd,LVM_GETITEMCOUNT,0,0);

    std::wstring wrow=utf8_to_w(r?r:"");
    std::wistringstream ws(wrow); std::wstring cell; int col=0;
    while(std::getline(ws,cell,L'\t')){
        if(col==0){
            LVITEMW item{}; item.mask=LVIF_TEXT; item.iItem=idx; item.iSubItem=0; item.pszText=(LPWSTR)cell.c_str();
            ListView_InsertItem(LB.hwnd,&item);
        }else{
            ListView_SetItemText(LB.hwnd,idx,col,(LPWSTR)cell.c_str());
        }
        ++col;
    }
    LB.LastAddedRowIndex=idx; LB.LastRowIndex=idx; LB.LastColumnIndex=col-1;
#else
    (void)h;(void)r;
#endif
}

XPLUGIN_API void XListbox_AddRowAt(int h,int at,const char* r){
#ifdef _WIN32
    std::lock_guard<std::mutex>lk(gMx); auto it=gObj.find(h); if(it==gObj.end()||!it->second->created)return;
    XListbox& LB=*it->second;
    std::wstring wrow=utf8_to_w(r?r:"");
    std::wistringstream ws(wrow); std::wstring cell; int col=0;
    while(std::getline(ws,cell,L'\t')){
        if(col==0){
            LVITEMW item{}; item.mask=LVIF_TEXT; item.iItem=at; item.iSubItem=0; item.pszText=(LPWSTR)cell.c_str();
            ListView_InsertItem(LB.hwnd,&item);
        }else{
            ListView_SetItemText(LB.hwnd,at,col,(LPWSTR)cell.c_str());
        }
        ++col;
    }
    LB.LastAddedRowIndex=at; LB.LastRowIndex=at; LB.LastColumnIndex=col-1;
#else
    (void)h;(void)at;(void)r;
#endif
}

XPLUGIN_API const char* XListbox_CellTextAt(int h,int row,int col){
#ifdef _WIN32
    std::lock_guard<std::mutex>lk(gMx); auto it=gObj.find(h); if(it==gObj.end()||!it->second->created)return strdup("");
    wchar_t buf[512]={0};
    ListView_GetItemText(it->second->hwnd,row,col,buf,511);
    std::string out=w_to_utf8(buf);
    return strdup(out.c_str());
#else
    (void)h;(void)row;(void)col; return strdup("");
#endif
}

XPLUGIN_API const char* XListbox_Content(int h){
#ifdef _WIN32
    std::lock_guard<std::mutex>lk(gMx); auto it=gObj.find(h); if(it==gObj.end()||!it->second->created)return strdup("");
    XListbox& LB=*it->second;
    int rows=(int)SendMessageW(LB.hwnd,LVM_GETITEMCOUNT,0,0);
    std::ostringstream out;
    for(int r=0;r<rows;++r){
        for(int c=0;c<LB.columnCount;++c){
            wchar_t buf[512]={0};
            ListView_GetItemText(LB.hwnd,r,c,buf,511);
            out<<w_to_utf8(buf);
            if(c<LB.columnCount-1) out<<'\t';
        }
        if(r<rows-1) out<<'\n';
    }
    return strdup(out.str().c_str());
#else
    (void)h; return strdup("");
#endif
}

XPLUGIN_API void XListbox_EditCellAt(int h,int row,int col,const char* t){
#ifdef _WIN32
    std::lock_guard<std::mutex>lk(gMx); auto it=gObj.find(h); if(it==gObj.end()||!it->second->created)return;
    std::wstring wtxt=utf8_to_w(t?t:"");
    ListView_SetItemText(it->second->hwnd,row,col,(LPWSTR)wtxt.c_str());
    it->second->LastRowIndex=row; it->second->LastColumnIndex=col;
#else
    (void)h;(void)row;(void)col;(void)t;
#endif
}

// HasBorder
XPLUGIN_API void XListbox_HasBorder_SET(int h, bool b) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    if(it!=gObj.end()) {
      it->second->HasBorder = b;
      // force repaint so border appears/disappears
      if(it->second->created)
        InvalidateRect(it->second->hwnd, nullptr, TRUE);
    }
}
XPLUGIN_API bool XListbox_HasBorder_GET(int h) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    return it!=gObj.end() ? it->second->HasBorder : false;
}

// BorderColor
XPLUGIN_API void XListbox_BorderColor_SET(int h, unsigned int c) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    if(it!=gObj.end()) it->second->BorderColor = (COLORREF)c;
}
XPLUGIN_API unsigned int XListbox_BorderColor_GET(int h) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    return it!=gObj.end() ? it->second->BorderColor : 0;
}

// TextColor (updates cell text color)
XPLUGIN_API void XListbox_TextColor_SET(int h, unsigned int c) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    if(it!=gObj.end()) {
      it->second->TextColor = (COLORREF)c;
      if(it->second->created) {
        SendMessageW(it->second->hwnd, LVM_SETTEXTCOLOR, (WPARAM)c, 0);
        InvalidateRect(it->second->hwnd, nullptr, TRUE);
      }
    }
}
XPLUGIN_API unsigned int XListbox_TextColor_GET(int h) {
    std::lock_guard<std::mutex> lk(gMx);
    auto it = gObj.find(h);
    return it!=gObj.end() ? it->second->TextColor : 0;
}

// returns the header text at column idx
XPLUGIN_API const char* XListbox_HeaderAt(int h, int col) {
    #ifdef _WIN32
        std::lock_guard<std::mutex> lk(gMx);
        auto it = gObj.find(h);
        if(it==gObj.end()||!it->second->created) return strdup("");
        LVCOLUMNW lc{}; 
        wchar_t buf[256]={0};
        lc.mask = LVCF_TEXT;
        lc.pszText = buf;
        lc.cchTextMax = 255;
        ListView_GetColumn(it->second->hwnd, col, &lc);
        auto s = w_to_utf8(buf);
        return strdup(s.c_str());
    #else
        return strdup("");
    #endif
    }
    
    // sets the header text at column idx
    XPLUGIN_API void XListbox_SetHeaderAt(int h, int col, const char* txt) {
    #ifdef _WIN32
        std::lock_guard<std::mutex> lk(gMx);
        auto it = gObj.find(h);
        if(it==gObj.end()||!it->second->created) return;
        std::wstring w = utf8_to_w(txt?txt:"");
        LVCOLUMNW lc{}; 
        lc.mask = LVCF_TEXT;
        lc.pszText = (LPWSTR)w.c_str();
        ListView_SetColumn(it->second->hwnd, col, &lc);
    #endif
    }

    // at file‐scope, alongside your other getters:
    XPLUGIN_API const char* XListbox_SelectionChanged_GET(int h) {
        // build the token that AddHandler will expect: "XListbox:<handle>:SelectionChanged"
        std::string tok = "XListbox:" + std::to_string(h) + ":SelectionChanged";
        return strdup(tok.c_str());
    }

    


// ── Event wiring ────────────────────────────────────────────────────────────
XPLUGIN_API bool XListbox_SetEventCallback(int h,const char* ev,void* cb){
    std::lock_guard<std::mutex>lk(gMx); if(!gObj.count(h)) return false;
    std::lock_guard<std::mutex>lk2(gEvMx); gEv[h][ev?ev:""]=cb; return true;
}

} // extern "C"

// ── triggerEvent implementation ─────────────────────────────────────────────
static void triggerEvent(int h,const std::string& ev,const char* param)
{
    void* cb=nullptr;
    { std::lock_guard<std::mutex>lk(gEvMx); auto it=gEv.find(h); if(it!=gEv.end()){auto jt=it->second.find(ev); if(jt!=it->second.end())cb=jt->second;} }
    if(!cb) return;
    char* p=strdup(param?param:"");
#ifdef _WIN32
    using CB=void(__stdcall*)(const char*);
#else
    using CB=void(*)(const char*);
#endif
    ((CB)cb)(p); free(p);
}

// ── minimal class-definition block for interpreter introspection ────────────
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

static ClassProperty props[]{
  {"Left","integer",(void*)XListbox_Left_GET,(void*)XListbox_Left_SET},
  {"Top","integer",(void*)XListbox_Top_GET,(void*)XListbox_Top_SET},
  {"Width","integer",(void*)XListbox_Width_GET,(void*)XListbox_Width_SET},
  {"Height","integer",(void*)XListbox_Height_GET,(void*)XListbox_Height_SET},
  {"Parent","integer",(void*)XListbox_Parent_GET,(void*)XListbox_Parent_SET},
  {"ColumnCount","integer",(void*)XListbox_ColumnCount_GET,(void*)XListbox_ColumnCount_SET},
  {"ColumnWidths","string",(void*)XListbox_ColumnWidths_GET,(void*)XListbox_ColumnWidths_SET},
  {"RowHeight","integer",(void*)XListbox_RowHeight_GET,(void*)XListbox_RowHeight_SET},
  {"Enabled","boolean",(void*)XListbox_Enabled_GET,(void*)XListbox_Enabled_SET},
  {"FontName","string",(void*)XListbox_FontName_GET,(void*)XListbox_FontName_SET},
  {"FontSize","integer",(void*)XListbox_FontSize_GET,(void*)XListbox_FontSize_SET},
  {"HasHeader","boolean",(void*)XListbox_HasHeader_GET,(void*)XListbox_HasHeader_SET},
  {"InitialValue","string",(void*)XListbox_InitialValue_GET,(void*)XListbox_InitialValue_SET},
  {"LastAddedRowIndex","integer",(void*)XListbox_LastAddedRowIndex_GET,nullptr},
  {"LastColumnIndex","integer",(void*)XListbox_LastColumnIndex_GET,nullptr},
  {"LastRowIndex","integer",(void*)XListbox_LastRowIndex_GET,nullptr},
  {"RowCount","integer",(void*)XListbox_RowCount_GET,nullptr},
  {"SelectedRow","integer",(void*)XListbox_SelectedRow_GET,(void*)XListbox_SelectedRow_SET},
  {"SelectionChanged","string",(void*)XListbox_SelectionChanged_GET,nullptr},
  {"HasBorder",   "boolean", (void*)XListbox_HasBorder_GET,   (void*)XListbox_HasBorder_SET},
  {"BorderColor", "color",   (void*)XListbox_BorderColor_GET, (void*)XListbox_BorderColor_SET},
  {"TextColor",   "color",   (void*)XListbox_TextColor_GET,   (void*)XListbox_TextColor_SET},
  {"Visible","boolean",(void*)XListbox_Visible_GET,(void*)XListbox_Visible_SET}
};
static ClassEntry methods[] = {
    { "AddRow",           (void*)XListbox_AddRow,           2, {"integer","string"},                     "void"    },
    { "AddRowAt",         (void*)XListbox_AddRowAt,         3, {"integer","integer","string"},           "void"    },
    { "CellTextAt",       (void*)XListbox_CellTextAt,       3, {"integer","integer","integer"},          "string"  },
    { "Content",          (void*)XListbox_Content,          1, {"integer"},                              "string"  },
    { "EditCellAt",       (void*)XListbox_EditCellAt,       4, {"integer","integer","integer","string"}, "void" },
    { "HeaderAt",         (void*)XListbox_HeaderAt,         2, {"integer","integer"},                    "string" },
    { "SetHeaderAt",       (void*)XListbox_SetHeaderAt,     3, {"integer","integer","string"},           "void" },
    { "XListbox_SetEventCallback", (void*)XListbox_SetEventCallback, 3, {"integer","string","pointer"},  "boolean" }
  };
  
static ClassDefinition classDef={"XListbox",sizeof(XListbox),(void*)Constructor,
                                 props,sizeof(props)/sizeof(props[0]),
                                 methods,sizeof(methods)/sizeof(methods[0])};

extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition(){ return &classDef; }

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD r, LPVOID){ if(r==DLL_PROCESS_DETACH){std::lock_guard<std::mutex>lk(gMx);for(auto&kv:gObj)delete kv.second;gObj.clear();}return TRUE;}
#else
__attribute__((destructor)) static void onUnload(){ std::lock_guard<std::mutex>lk(gMx);for(auto&kv:gObj)delete kv.second;gObj.clear();}
#endif
