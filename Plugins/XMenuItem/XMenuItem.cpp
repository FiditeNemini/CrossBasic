/*

  XMenuItem.cpp
  CrossBasic Plugin: XMenuItem                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
#include <string>
#include <cstdlib>
#include <random>
#include <algorithm>          // <- for std::transform
#include <atomic>

#ifdef _WIN32
  #include <windows.h>
  #include <commctrl.h>
  #pragma comment(lib,"comctl32.lib")
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

/* ─── after the other #include's & globals ─────────────────────────── */
#ifdef _WIN32
#include <unordered_set>
static std::mutex               gSubMx;
static std::unordered_set<HWND> gSubclassed;
#endif


/* ===== Helpers imported from XMenuBar.dll ================================= */
#ifdef _WIN32
extern "C" {
  typedef HMENU (*FnGetHMenu)(int);
  typedef HWND  (*FnGetHWND)(int);
  static FnGetHMenu XMenuBar_GetHMenu=nullptr;
  static FnGetHWND  XMenuBar_GetHWND =nullptr;
  static void LoadBarFns(){
      if (XMenuBar_GetHMenu && XMenuBar_GetHWND) return;
      HMODULE m=GetModuleHandleA("XMenuBar.dll");
      if (!m) return;
      XMenuBar_GetHMenu=(FnGetHMenu)GetProcAddress(m,"XMenuBar_GetHMenu");
      XMenuBar_GetHWND =(FnGetHWND )GetProcAddress(m,"XMenuBar_GetHWND");
  }
}
#endif

/* ===== XMenuItem instance ================================================== */
class XMenuItem{
public:
    int handle;
    int parentHandle{0};
#ifdef _WIN32
    HMENU   hParentMenu{nullptr};
    HMENU   hSubMenu{nullptr};
    HWND    hwnd{nullptr};
    UINT_PTR cmdId{0};
#endif
    bool isSeparator{false};
    bool created{false};
    bool subclassed{false};
    std::string caption;
    explicit XMenuItem(int h):handle(h){
    #ifdef _WIN32
      hSubMenu=CreatePopupMenu();
    #endif
    }
    ~XMenuItem(){
    #ifdef _WIN32
      if(hSubMenu) DestroyMenu(hSubMenu);
    #endif
    }
};

/* ===== Globals ============================================================ */
static std::mutex                      gMx;
static std::unordered_map<int,XMenuItem*> gItems;
static std::mt19937                    gRng(std::random_device{}());
static std::uniform_int_distribution<int> gDist(10000000,99999999);

#ifdef _WIN32
static std::mutex                      gCmdMx;
static std::unordered_map<UINT_PTR,int> gCmdMap;
static std::atomic<WORD>               gNextCmdId{1};   // *** NEW ***
#endif

static std::mutex                      gEvtMx;
static std::unordered_map<int,std::unordered_map<std::string,void*>> gEvts;

/* ===== Internal: trigger "pressed" handlers =============================== */
static void triggerPressed(int h){
    void* cb=nullptr;
    { std::lock_guard<std::mutex> lk(gEvtMx);
      auto it=gEvts.find(h);
      if(it!=gEvts.end()){
        auto jt=it->second.find("pressed");
        if(jt!=it->second.end()) cb=jt->second;
      }
    }
    if(!cb) return;
#ifdef _WIN32
    using CB = void(__stdcall*)(const char*);
#else
    using CB = void(*)(const char*);
#endif
    ((CB)cb)("");
}

/* ===== WM_COMMAND dispatcher ============================================= */
#ifdef _WIN32
static LRESULT CALLBACK MenuWndProc(HWND h,UINT m,WPARAM wp,LPARAM lp,
                                    UINT_PTR id,DWORD_PTR){
    if(m==WM_COMMAND){
        UINT_PTR cid=LOWORD(wp);
        std::lock_guard<std::mutex> lk(gCmdMx);
        auto it=gCmdMap.find(cid);
        if(it!=gCmdMap.end()) triggerPressed(it->second);
    }
    return DefSubclassProc(h,m,wp,lp);
}
#endif

/* ===== House-keeping ====================================================== */
static void Cleanup(){
    std::lock_guard<std::mutex> lk(gMx);
    for(auto &kv:gItems) delete kv.second;
    gItems.clear();
}

extern "C" {

/* █████ Constructor / Close █████ */
XPLUGIN_API int Constructor(){
    std::lock_guard<std::mutex> lk(gMx);
    int h; do{ h=gDist(gRng);}while(gItems.count(h));
    gItems[h]=new XMenuItem(h); return h;
}
XPLUGIN_API void Close(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gItems.find(h); if(it!=gItems.end()){ delete it->second; gItems.erase(it);}
}

/* █████ handle (lower-case) █████ */
XPLUGIN_API int handle_GET(int h){ return h; }

/* █████ Parent (integer) █████ */
XPLUGIN_API void Parent_SET(int h,int ph){
#ifdef _WIN32
    LoadBarFns();
#endif
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gItems.find(h); if(it==gItems.end()) return;
    XMenuItem* me=it->second; me->parentHandle=ph;
#ifdef _WIN32
    if (XMenuBar_GetHMenu && XMenuBar_GetHMenu(ph)){
        me->hParentMenu=XMenuBar_GetHMenu(ph);
        me->hwnd       =XMenuBar_GetHWND(ph);
    }else if(auto pit=gItems.find(ph); pit!=gItems.end()){
        me->hParentMenu=pit->second->hSubMenu;
        me->hwnd       =pit->second->hwnd;
    }else{
        using Fn=HWND(*)(int);
        static Fn GetWnd=nullptr;
        if(!GetWnd){
          HMODULE m=GetModuleHandleA("XWindow.dll");
          if(m) GetWnd=(Fn)GetProcAddress(m,"XWindow_GetHWND");
        }
        me->hwnd=GetWnd?GetWnd(ph):nullptr;
        if(me->hwnd){
            me->hParentMenu=GetMenu(me->hwnd);
            if(!me->hParentMenu){
                me->hParentMenu=CreateMenu();
                SetMenu(me->hwnd,me->hParentMenu);
            }
        }
    }

    #ifdef _WIN32
    /* sub-class this window only once, no matter how many menu-items exist */
    if (me->hwnd) {
        std::lock_guard<std::mutex> lkSub(gSubMx);
        if (gSubclassed.insert(me->hwnd).second) {        // newly inserted
            SetWindowSubclass(me->hwnd, MenuWndProc, 1 /*single ID*/, 0);
        }
    }
    #endif

#endif
}
XPLUGIN_API int Parent_GET(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gItems.find(h);
    return it!=gItems.end()?it->second->parentHandle:0;
}

/* █████ IsSeparator █████ */
XPLUGIN_API void IsSeparator_SET(int h,bool v){
    std::lock_guard<std::mutex> lk(gMx);
    if(gItems.count(h)) gItems[h]->isSeparator=v;
}
XPLUGIN_API bool IsSeparator_GET(int h){
    std::lock_guard<std::mutex> lk(gMx);
    return gItems.count(h)?gItems[h]->isSeparator:false;
}

/* █████ Caption █████ */
XPLUGIN_API void Caption_SET(int h,const char* txt){
#ifdef _WIN32
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gItems.find(h); if(it==gItems.end()) return;
    XMenuItem* me=it->second; me->caption=txt?txt:"";
    HMENU pm=me->hParentMenu; if(!pm||!me->hwnd) return;

    if(me->isSeparator){
        AppendMenuA(pm,MF_SEPARATOR,0,nullptr);
        DrawMenuBar(me->hwnd);
        return;
    }
    if(!me->created){
        if(gItems.count(me->parentHandle)==0){               // root menu
            AppendMenuA(pm,MF_POPUP,(UINT_PTR)me->hSubMenu,me->caption.c_str());
        }else{                                               // leaf
            me->cmdId = gNextCmdId.fetch_add(1);             // ***16-bit IDs***
            if(me->cmdId==0) me->cmdId = gNextCmdId.fetch_add(1);
            {
              std::lock_guard<std::mutex> ck(gCmdMx);
              gCmdMap[me->cmdId]=h;
            }
            AppendMenuA(pm,MF_STRING,me->cmdId,me->caption.c_str());
        }
        me->created=true;
        DrawMenuBar(me->hwnd);
    }
#endif
}
XPLUGIN_API const char* Caption_GET(int h){
    std::lock_guard<std::mutex> lk(gMx);
    return gItems.count(h)?strdup(gItems[h]->caption.c_str()):strdup("");
}

/* █████ Pressed token (lower-case) █████ */
XPLUGIN_API const char* Pressed_GET(int h){
    std::string s="XMenuItem:"+std::to_string(h)+":pressed";
    return strdup(s.c_str());
}

/* █████ Event hookup █████ */
XPLUGIN_API bool XMenuItem_SetEventCallback(int h,const char* ev,void* fp){
    std::string key(ev?ev:"");
    if(auto p=key.rfind(':'); p!=std::string::npos) key.erase(0,p+1);
    std::transform(key.begin(),key.end(),key.begin(),::tolower);
    std::lock_guard<std::mutex> lk(gEvtMx);
    gEvts[h][key]=fp;
    return true;
}

/* ===== Class definition =================================================== */
typedef struct {const char* n;const char* t;void* g;void* s;} Prop;
typedef struct {const char* n;void* f;int a;const char* p[10];const char* r;} Meth;
typedef struct {const char* n;size_t sz;void* ctor;Prop* p;size_t pc;
                Meth* m;size_t mc;void* c;size_t cc;} ClassDef;

static Prop props[]{
  {"handle","integer",(void*)handle_GET,nullptr},
  {"Parent","integer",(void*)Parent_GET,(void*)Parent_SET},
  {"Caption","string",(void*)Caption_GET,(void*)Caption_SET},
  {"IsSeparator","boolean",(void*)IsSeparator_GET,(void*)IsSeparator_SET},
  {"Pressed","string",(void*)Pressed_GET,nullptr}
};
static Meth meths[]{
  {"XMenuItem_SetEventCallback",(void*)XMenuItem_SetEventCallback,3,
      {"integer","string","pointer"},"boolean"}
};
static ClassDef cd={
  "XMenuItem",sizeof(XMenuItem),(void*)Constructor,
  props,sizeof(props)/sizeof(props[0]),
  meths,sizeof(meths)/sizeof(meths[0]),
  nullptr,0
};
XPLUGIN_API ClassDef* GetClassDefinition(){ return &cd; }

/* ===== DLL entry / exit =================================================== */
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD r, LPVOID){
    if(r==DLL_PROCESS_DETACH) Cleanup();
    return TRUE;
}
#else
__attribute__((destructor)) static void onUnload(){ Cleanup();}
#endif

} /* extern "C" */
