/*

  XCanvas.cpp
  CrossBasic Plugin: XCanvas                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <mutex>
#include <unordered_map>
#include <random>
#include <string>
#include <iostream>

#define XPLUGIN_API __declspec(dllexport)
#define strdup _strdup

/* ------------------------------------------------------------------------
   Dynamic bindings
------------------------------------------------------------------------ */
static HMODULE             gGfxLib = nullptr;
typedef void  (__cdecl* GfxCloseFn)(int);
typedef void  (__cdecl* GfxBlitFn)(int,HDC);
typedef void  (__cdecl* GfxDrawPicFn)(int,int,int,int,int,int);
typedef int   (__cdecl* GfxCtorWndFn)(int);
typedef int   (__cdecl* GfxCtorPicFn)(int,int,int);

static GfxCloseFn          pGfx_Close             = nullptr;
static GfxBlitFn           pGfx_Blit              = nullptr;
static GfxDrawPicFn        pGfx_DrawPicture       = nullptr;
static GfxCtorWndFn        pGfx_Constructor       = nullptr;
static GfxCtorPicFn        pGfx_ConstructorPicture= nullptr;   // single definition!

/* --- XPicture bridge --- */
static HMODULE  gPicLib           = nullptr;
typedef int (__cdecl* PicGfxGetFn)(int);
static PicGfxGetFn pPic_Graphics_GET = nullptr;

/* -------------------------------------------------------------------- */
static bool EnsureGfxBindings()
{
    if (gGfxLib) return true;
    // gGfxLib = LoadLibraryA("XGraphics.dll");
    // if (!gGfxLib) { std::cerr<<"XCanvas: cannot load XGraphics.dll\n"; return false; }
    gGfxLib = GetModuleHandleA("XGraphics.dll");
    if (!gGfxLib)                       // not yet? then load it
        gGfxLib = LoadLibraryA("XGraphics.dll");

    if (!gGfxLib) { std::cerr << "...cannot load XGraphics.dll\n"; return false; }

    pGfx_Close              = (GfxCloseFn) GetProcAddress(gGfxLib,"XGraphics_Close");
    pGfx_Blit               = (GfxBlitFn)  GetProcAddress(gGfxLib,"XGraphics_Blit");
    pGfx_DrawPicture        = (GfxDrawPicFn)GetProcAddress(gGfxLib,"XGraphics_DrawPicture");
    pGfx_Constructor        = (GfxCtorWndFn)GetProcAddress(gGfxLib,"XGraphics_Constructor");
    pGfx_ConstructorPicture = (GfxCtorPicFn)GetProcAddress(gGfxLib,"XGraphics_ConstructorPicture");

    if(!pGfx_Close||!pGfx_Blit||!pGfx_DrawPicture||!pGfx_Constructor||!pGfx_ConstructorPicture){
        std::cerr<<"XCanvas: missing exports in XGraphics.dll\n"; return false;
    }
    return true;
}
static bool EnsurePicBindings()
{
    if (gPicLib) return true;
    gPicLib = LoadLibraryA("XPicture.dll");
    if (!gPicLib) { std::cerr<<"XCanvas: cannot load XPicture.dll\n"; return false; }
    pPic_Graphics_GET = (PicGfxGetFn)GetProcAddress(gPicLib,"XPicture_Graphics_GET");
    return (bool)pPic_Graphics_GET;
}

/* -------------------------------------------------------------------- */
/*                     Per-instance state & helpers                     */
struct CanvasInst{
    int     gfxHandle   = 0;         // our double-buffer
    int     backdropGfx = 0;         // optional background picture (graphics handle)
    HWND    hwnd        = nullptr;
    int     x=0,y=0,w=0,h=0,parent=0;
    bool    created     = false;
};

static std::mutex                                      gMx;
static std::unordered_map<int,CanvasInst*>             gInst;
static std::mt19937                                    gRng{std::random_device{}()};
static std::uniform_int_distribution<int>              gDist(10000000,99999999);

/* -------- event dispatch -------- */
static std::mutex gEvMx;
static std::unordered_map<int,std::unordered_map<std::string,void*>> gCallbacks;

static void fire(int h,const std::string& ev,const char* param)
{
    void* fp=nullptr;
    { std::lock_guard<std::mutex> lk(gEvMx);
      auto it=gCallbacks.find(h);
      if(it!=gCallbacks.end()){
        auto jt=it->second.find(ev);
        if(jt!=it->second.end()) fp=jt->second;
      }}
    if(!fp) return;
    using CB = void(__stdcall*)(const char*);
    ((CB)fp)(param?param:"");
}

/* -------------------------------------------------------------------- */
/*                    Win32 subclass – real painting                    */
#ifdef _WIN32
static LRESULT CALLBACK CanvasProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,
                                   UINT_PTR, DWORD_PTR ref)
{
    int h=(int)ref;
    CanvasInst* ci=gInst[h];
    switch(msg){
    /* case WM_SIZE:{
        ci->w=LOWORD(lp); ci->h=HIWORD(lp);
        if(ci->gfxHandle) pGfx_Close(ci->gfxHandle);
        ci->gfxHandle = pGfx_ConstructorPicture(ci->w?ci->w:1,ci->h?ci->h:1,32);
        break;
    } */
    /* case WM_SIZE:
    {
        RECT rc;
        GetClientRect(hwnd, &rc);           // <-- always valid
        ci->w = rc.right  - rc.left;
        ci->h = rc.bottom - rc.top;

        if (ci->gfxHandle) pGfx_Close(ci->gfxHandle);
        ci->gfxHandle =
            pGfx_ConstructorPicture(ci->w ? ci->w : 1,
                                    ci->h ? ci->h : 1,
                                    32);
        break;
    } */
   case WM_SIZE:
    {
        /* Always ask the control itself – this is never 0 × 0 */
        RECT rc;
        GetClientRect(hwnd, &rc);

        ci->w = rc.right  - rc.left;
        ci->h = rc.bottom - rc.top;

        /* rebuild back-buffer */
        if (ci->gfxHandle) pGfx_Close(ci->gfxHandle);
        ci->gfxHandle = pGfx_ConstructorPicture(
                            ci->w ? ci->w : 1,
                            ci->h ? ci->h : 1,
                            32);
        break;
    }

    case WM_PAINT:{
        PAINTSTRUCT ps; HDC dc=BeginPaint(hwnd,&ps);

        /* draw optional backdrop */
        if(ci->backdropGfx)
            pGfx_DrawPicture(ci->gfxHandle,ci->backdropGfx,0,0,ci->w,ci->h);

        /* user Paint event */
        char buf[32]; sprintf(buf,"%d",ci->gfxHandle);
        fire(h,"paint",buf);

        pGfx_Blit(ci->gfxHandle,dc);
        EndPaint(hwnd,&ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {char b[32]; sprintf(b,"%d,%d",GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); fire(h,"mousedown",b); break;}
    case WM_LBUTTONUP:   {char b[32]; sprintf(b,"%d,%d",GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); fire(h,"mouseup",b);   break;}
    case WM_MOUSEMOVE:   {char b[32]; sprintf(b,"%d,%d",GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); fire(h,"mousemove",b); break;}
    case WM_LBUTTONDBLCLK:{char b[32];sprintf(b,"%d,%d",GET_X_LPARAM(lp),GET_Y_LPARAM(lp)); fire(h,"doubleclick",b);break;}
    }
    return DefSubclassProc(hwnd,msg,wp,lp);
}
#endif

/* -------------------------------------------------------------------- */
/*                              API                                     */
extern "C"{

XPLUGIN_API int  XCanvas_Constructor()
{
    std::lock_guard<std::mutex> lk(gMx);
    int h; do{h=gDist(gRng);}while(gInst.count(h));
    gInst[h]=new CanvasInst;
    return h;
}
XPLUGIN_API void XCanvas_Close(int h)
{
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h); if(it==gInst.end())return;
    if(it->second->gfxHandle) pGfx_Close(it->second->gfxHandle);
    delete it->second; gInst.erase(it);
}

/* ---- simple properties (Left/Top/Width/Height) ---- */
#define PROP(NAME,member)                                              \
XPLUGIN_API int  XCanvas_##NAME##_GET(int h){std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h);return it!=gInst.end()?it->second->member:0;} \
XPLUGIN_API void XCanvas_##NAME##_SET(int h,int v){std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h);if(it==gInst.end())return;it->second->member=v; if(it->second->created) MoveWindow(it->second->hwnd,it->second->x,it->second->y,it->second->w,it->second->h,TRUE);}
PROP(X,x) PROP(Y,y) PROP(Width,w) PROP(Height,h)

/* ---- Parent (creates the real HWND) -------------------------------- */
XPLUGIN_API void XCanvas_Parent_SET(int h,int parent)
{
    std::lock_guard<std::mutex> lk(gMx);
    auto ci=gInst[h];
    if(ci->created){ DestroyWindow(ci->hwnd); ci->created=false; pGfx_Close(ci->gfxHandle); ci->gfxHandle=0; }
    ci->parent = parent;

    using Fn=HWND(*)(int); static Fn getHWND=nullptr;
    if(!getHWND){ HMODULE m=GetModuleHandleA("XWindow.dll"); if(m) getHWND=(Fn)GetProcAddress(m,"XWindow_GetHWND"); }
    HWND phwnd = getHWND?getHWND(parent):nullptr; if(!phwnd) return;

    ci->hwnd = CreateWindowExW(0,L"STATIC",L"",WS_CHILD|WS_VISIBLE,
                               ci->x,ci->y,ci->w,ci->h,phwnd,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!ci->hwnd) return;
    ci->created=true;

    EnsureGfxBindings();
    ci->gfxHandle = pGfx_ConstructorPicture(ci->w?ci->w:1,ci->h?ci->h:1,32);

    SetWindowSubclass(ci->hwnd,CanvasProc,0,(DWORD_PTR)h);
}
XPLUGIN_API int  XCanvas_Parent_GET(int h){std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h);return it!=gInst.end()?it->second->parent:0;}

/* ---- exposed handles ----------------------------------------------- */
XPLUGIN_API int  XCanvas_Graphics_GET (int h){std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h);return it!=gInst.end()?it->second->gfxHandle:0;}
XPLUGIN_API void XCanvas_Backdrop_SET(int h,int picH){ if(!EnsurePicBindings())return; std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h); if(it==gInst.end())return; it->second->backdropGfx = pPic_Graphics_GET(picH); if(it->second->created) InvalidateRect(it->second->hwnd,nullptr,TRUE);}
XPLUGIN_API int  XCanvas_Backdrop_GET(int h){std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h);return it!=gInst.end()?it->second->backdropGfx:0;}

/* ---- misc ----------------------------------------------------------- */
XPLUGIN_API void XCanvas_Refresh   (int h){std::lock_guard<std::mutex>lk(gMx);auto it=gInst.find(h);if(it!=gInst.end()&&it->second->created)InvalidateRect(it->second->hwnd,nullptr,TRUE);}
XPLUGIN_API void XCanvas_Invalidate(int h){XCanvas_Refresh(h);}

/* ---- event token getters ------------------------------------------- */
#define TOKEN_GETTER(name,evt) XPLUGIN_API const char* XCanvas_##name##_GET(int h){std::string s="xcanvas:"+std::to_string(h)+":"+evt;return strdup(s.c_str());}
TOKEN_GETTER(Paint,"paint") TOKEN_GETTER(MouseDown,"mousedown") TOKEN_GETTER(MouseUp,"mouseup")
TOKEN_GETTER(MouseMove,"mousemove") TOKEN_GETTER(DoubleClick,"doubleclick")

XPLUGIN_API bool XCanvas_SetEventCallback(int h,const char* token,void* fp)
{
    { std::lock_guard<std::mutex> lk(gMx); if(!gInst.count(h)) return false; }
    std::string key=token?token:"";
    if(auto p=key.rfind(':');p!=std::string::npos) key.erase(0,p+1);
    std::lock_guard<std::mutex>lk(gEvMx); gCallbacks[h][key]=fp; return true;
}

/* -------------------------------------------------------------------- */
/*         CrossBasic registration – now publishes event constants      */
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

static ClassConstant kConsts[] = {
};


static ClassProperty kProps[]={
    {"Left","integer",(void*)XCanvas_X_GET,(void*)XCanvas_X_SET},
    {"Top","integer",(void*)XCanvas_Y_GET,(void*)XCanvas_Y_SET},
    {"Width","integer",(void*)XCanvas_Width_GET,(void*)XCanvas_Width_SET},
    {"Height","integer",(void*)XCanvas_Height_GET,(void*)XCanvas_Height_SET},
    {"Parent","integer",(void*)XCanvas_Parent_GET,(void*)XCanvas_Parent_SET},
    {"Graphics","XGraphics",(void*)XCanvas_Graphics_GET,nullptr},
    {"Backdrop","XPicture",(void*)XCanvas_Backdrop_GET,(void*)XCanvas_Backdrop_SET},
        /* ── event-token properties (restore these!) ───────────────────── */
        {"Paint",       "string",  (void*)XCanvas_Paint_GET,       nullptr},
        {"MouseDown",   "string",  (void*)XCanvas_MouseDown_GET,   nullptr},
        {"MouseUp",     "string",  (void*)XCanvas_MouseUp_GET,     nullptr},
        {"MouseMove",   "string",  (void*)XCanvas_MouseMove_GET,   nullptr},
        {"DoubleClick", "string",  (void*)XCanvas_DoubleClick_GET, nullptr}
};
static ClassEntry kMethods[]={
    {"Refresh",   (void*)XCanvas_Refresh,    1,{"integer"},"void"},
    {"Invalidate",(void*)XCanvas_Invalidate, 1,{"integer"},"void"},
    {"XCanvas_SetEventCallback",(void*)XCanvas_SetEventCallback,3,{"integer","string","pointer"},"boolean"}
};
typedef struct{
    const char*     className;
    size_t          classSize;
    void*           ctor;
    ClassProperty*         props; size_t propsCnt;
    ClassEntry*        meths; size_t methCnt;
    ClassConstant*         consts;size_t constCnt;
} CBClassDef;

static ClassDefinition gDef={
    "XCanvas",sizeof(CanvasInst),
    (void*)XCanvas_Constructor,
    kProps,   sizeof(kProps  )/sizeof(kProps[0]),
    kMethods, sizeof(kMethods)/sizeof(kMethods[0]),
    kConsts,  sizeof(kConsts )/sizeof(kConsts[0])
};

XPLUGIN_API ClassDefinition* GetClassDefinition(){ return &gDef; }

/* ---- DLL unload ---- */
static void CleanupAll(){
    std::lock_guard<std::mutex> lk(gMx);
    for(auto&kv:gInst){ if(kv.second->gfxHandle) pGfx_Close(kv.second->gfxHandle); delete kv.second; }
    gInst.clear(); if(gGfxLib) FreeLibrary(gGfxLib); if(gPicLib) FreeLibrary(gPicLib);
}
BOOL APIENTRY DllMain(HMODULE, DWORD r, LPVOID){ if(r==DLL_PROCESS_DETACH) CleanupAll(); return TRUE; }

} // extern "C"
