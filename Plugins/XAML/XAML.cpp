//REQUIRES MSVC
/*

  XamlContainer.cpp
  CrossBasic Plugin: XamlContainer                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
// © 2025 Simulanics Technologies / Matthew A Combatti (MIT)
// clang++ -std=c++20 -MD -m64 -shared -o XAML.dll XAML.cpp -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32 -lwindowsapp
/* ————— STANDARD WINDOWS + WINRT HEADERS ————— */
#ifdef _WIN32
# define UNICODE
# define _UNICODE
# include <windows.h>
# undef  GetCurrentTime
# include <coroutine>
# include <objbase.h>
# include <winrt/base.h>
# include <winrt/Windows.UI.Xaml.h>
# include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Controls.Primitives.h>
# include <winrt/Windows.UI.Xaml.Input.h>
# include <winrt/Windows.UI.Xaml.Markup.h>
# include <winrt/Windows.UI.Xaml.Hosting.h>
# pragma comment(lib,"runtimeobject.lib")
# pragma comment(lib,"windowsapp")
# define XPLUGIN_API __declspec(dllexport)
#else
# define XPLUGIN_API __attribute__((visibility("default")))
#endif

/* ————— FORWARD COM IID FOR XAML ISLAND ————— */
#ifdef _WIN32
MIDL_INTERFACE("3CBCF1BF-2F76-4E9C-96AB-E84B37972554")
IDesktopWindowXamlSourceNative : public IUnknown {
    virtual HRESULT __stdcall AttachToWindow(HWND) noexcept = 0;
    virtual HRESULT __stdcall get_WindowHandle(HWND *) noexcept = 0;
};
#endif

/* ——— STL ——— */
#include <mutex>
#include <unordered_map>
#include <random>
#include <atomic>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>

/* ————— UTF-8 / UTF-16 HELPERS ————— */
static std::wstring utf8_to_wstring(const char* s){
    if(!s) return L"";
    int n=MultiByteToWideChar(CP_UTF8,0,s,-1,nullptr,0);
    std::wstring w(n,L'\0');
    MultiByteToWideChar(CP_UTF8,0,s,-1,&w[0],n);
    if(!w.empty()&&w.back()==L'\0') w.pop_back();
    return w;
}
static std::string wstring_to_utf8(const std::wstring& w){
    if(w.empty()) return "";
    int n=WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,nullptr,0,nullptr,nullptr);
    std::string s(n,'\0');
    WideCharToMultiByte(CP_UTF8,0,w.c_str(),-1,&s[0],n,nullptr,nullptr);
    if(!s.empty()&&s.back()=='\0') s.pop_back();
    return s;
}

/* ————— forward dispatcher ————— */
static void dispatchEvent(int, const std::string&, const char*);

/* ————— Container class ————— */
class XamlContainer {
public:
    int handle;
    int x{0},y{0},width{300},height{200};
    int parentHandle{0};
    std::wstring xamlString;

#ifdef _WIN32
    HWND hostHwnd{nullptr};
    winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource xamlSource{nullptr};
    winrt::com_ptr<IDesktopWindowXamlSourceNative> xamlSourceNative{nullptr};
    bool created{false};
#endif
    std::unordered_map<std::string,void*> xamlEvents;
    std::mutex eventsMx;

    explicit XamlContainer(int h):handle(h){}
    ~XamlContainer(){
#ifdef _WIN32
        if(created && hostHwnd) DestroyWindow(hostHwnd);
#endif
    }
};

/* ————— Globals ————— */
static std::mutex gInstMx;
static std::unordered_map<int,XamlContainer*> gInst;
static std::mt19937 rng(std::random_device{}());
static std::uniform_int_distribution<int> dist(10000000,99999999);

#ifdef _WIN32
static std::atomic<int> gNextId{2000};

/* parent HWND helper */
static HWND GetParentHWND(int ph){
    using Fn=HWND(*)(int);
    static Fn fn=nullptr;
    if(!fn){
        if(HMODULE m=GetModuleHandleA("XWindow.dll"))
            fn=reinterpret_cast<Fn>(GetProcAddress(m,"XWindow_GetHWND"));
    }
    return fn?fn(ph):nullptr;
}

/* hook single Element.Event */
static void HookSingle(XamlContainer& C,const std::string& key){
    auto p=key.find('.'); if(p==std::string::npos) return;
    auto elem=key.substr(0,p); auto ev=key.substr(p+1);
    auto root=C.xamlSource.Content().try_as<winrt::Windows::UI::Xaml::FrameworkElement>();
    if(!root) return;
    winrt::hstring h{utf8_to_wstring(elem.c_str())};

    if(ev=="Click"){
        if(auto btn=root.FindName(h).try_as<winrt::Windows::UI::Xaml::Controls::Button>()){
            int hnd=C.handle; std::string keyCopy=key;
            {
                winrt::event_token tok = btn.Click(
                    [hnd,keyCopy](auto&&,auto&&){ dispatchEvent(hnd,keyCopy,nullptr); });
                (void)tok;               // silence ‘nodiscard’ warning
            }
        }
    }else if(ev=="Tapped"){
        if(auto ui=root.FindName(h).try_as<winrt::Windows::UI::Xaml::UIElement>()){
            int hnd=C.handle; std::string keyCopy=key;
            {
                winrt::event_token tok = ui.Tapped(
                    [hnd,keyCopy](auto&&,auto&&){ dispatchEvent(hnd,keyCopy,nullptr); });
                (void)tok;
            }
        }
    }
}
static void HookAll(XamlContainer& C){
    std::lock_guard<std::mutex> l(C.eventsMx);
    for(auto& kv:C.xamlEvents) HookSingle(C,kv.first);
}

/* cleanup */
static void Cleanup(){
    std::lock_guard<std::mutex> l(gInstMx);
    for(auto& kv:gInst) delete kv.second;
    gInst.clear();
    winrt::uninit_apartment();
}
#endif  // _WIN32

/* ————— Exported C API ————— */
extern "C"{

XPLUGIN_API int Constructor(){
    int h;
    { std::lock_guard<std::mutex> l(gInstMx);
      do{h=dist(rng);}while(gInst.count(h));
      gInst[h]=new XamlContainer(h);
    }
    return h;
}
XPLUGIN_API void Close(int h){
    std::lock_guard<std::mutex> l(gInstMx);
    if(auto it=gInst.find(h); it!=gInst.end()){
        delete it->second; gInst.erase(it);
    }
}

/* property helpers */
#define PROP_INT(name,field) \
XPLUGIN_API void XamlContainer_##name##_SET(int h,int v){ \
    std::lock_guard<std::mutex> l(gInstMx); \
    if(auto it=gInst.find(h); it!=gInst.end()){ it->second->field=v; \
        if(it->second->created) MoveWindow(it->second->hostHwnd, \
            it->second->x,it->second->y,it->second->width,it->second->height,TRUE);} } \
XPLUGIN_API int XamlContainer_##name##_GET(int h){ \
    std::lock_guard<std::mutex> l(gInstMx); \
    if(auto it=gInst.find(h); it!=gInst.end()) return it->second->field; \
    return 0; }
PROP_INT(Left,x) PROP_INT(Top,y) PROP_INT(Width,width) PROP_INT(Height,height)
#undef PROP_INT

/* Parent property – creates island */
XPLUGIN_API void XamlContainer_Parent_SET(int h,int ph){
    std::lock_guard<std::mutex> l(gInstMx);
    auto it=gInst.find(h); if(it==gInst.end()) return;
#ifdef _WIN32
    auto& C=*it->second;
    if(C.created && C.hostHwnd) DestroyWindow(C.hostHwnd);

    HWND p=GetParentHWND(ph);
    if(!p) return;
    C.parentHandle=ph;
    static bool inited=false;
    if(!inited){ winrt::init_apartment(); inited=true; }

    C.xamlSource = winrt::Windows::UI::Xaml::Hosting::DesktopWindowXamlSource();
    C.xamlSource.as(IID_PPV_ARGS(C.xamlSourceNative.put()));
    C.xamlSourceNative->AttachToWindow(p);
    C.xamlSourceNative->get_WindowHandle(&C.hostHwnd);
    C.created=true;
    MoveWindow(C.hostHwnd,C.x,C.y,C.width,C.height,TRUE);
#endif
}
XPLUGIN_API int XamlContainer_Parent_GET(int h){
    std::lock_guard<std::mutex> l(gInstMx);
    if(auto it=gInst.find(h); it!=gInst.end()) return it->second->parentHandle;
    return 0;
}

/* LoadXaml */
XPLUGIN_API void XamlContainer_LoadXaml(int h,const char* xaml){
    std::lock_guard<std::mutex> l(gInstMx);
    auto it=gInst.find(h); if(it==gInst.end()) return;
#ifdef _WIN32
    auto& C=*it->second; if(!C.created) return;
    C.xamlString=utf8_to_wstring(xaml);
    auto ui=winrt::Windows::UI::Xaml::Markup::XamlReader::Load(C.xamlString)
             .as<winrt::Windows::UI::Xaml::UIElement>();
    C.xamlSource.Content(ui);
    HookAll(C);
    dispatchEvent(h,"Loaded",nullptr);
#endif
}
XPLUGIN_API void XamlContainer_Xaml_SET(int h,const char* s){ XamlContainer_LoadXaml(h,s); }
XPLUGIN_API const char* XamlContainer_Xaml_GET(int h){
    std::lock_guard<std::mutex> l(gInstMx);
    if(auto it=gInst.find(h); it!=gInst.end()){
        return _strdup(wstring_to_utf8(it->second->xamlString).c_str());
    }
    return _strdup("");
}

/* AddXamlEvent */
XPLUGIN_API bool XamlContainer_AddXamlEvent(int h,const char* elem,const char* ev,void* cb){
    std::lock_guard<std::mutex> l(gInstMx);
    auto it=gInst.find(h); if(it==gInst.end()) return false;
    auto& C=*it->second;
    std::string key=std::string(elem)+"."+std::string(ev);
    { std::lock_guard<std::mutex> l2(C.eventsMx); C.xamlEvents[key]=cb; }
#ifdef _WIN32
    if(C.created)
        HookSingle(C,key);
#endif
    return true;
}

/* Loaded token */
XPLUGIN_API const char* XamlContainer_Loaded_GET(int h){
    std::ostringstream os; os<<"xamlcontainer:"<<h<<":Loaded";
    return _strdup(os.str().c_str());
}


// Class and method registration
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
    { "Left",    "integer", (void*)XamlContainer_Left_GET,    (void*)XamlContainer_Left_SET },
    { "Top",     "integer", (void*)XamlContainer_Top_GET,     (void*)XamlContainer_Top_SET },
    { "Width",   "integer", (void*)XamlContainer_Width_GET,   (void*)XamlContainer_Width_SET },
    { "Height",  "integer", (void*)XamlContainer_Height_GET,  (void*)XamlContainer_Height_SET },
    { "Parent",  "integer", (void*)XamlContainer_Parent_GET,  (void*)XamlContainer_Parent_SET },
    { "Xaml",    "string",  (void*)XamlContainer_Xaml_GET,    (void*)XamlContainer_Xaml_SET },
    { "Loaded",  "string",  (void*)XamlContainer_Loaded_GET,  nullptr }
};

static ClassEntry methods[] = {
    { "LoadXaml",      (void*)XamlContainer_LoadXaml,      2, {"integer","string"},                  "void"    },
    { "AddXamlEvent",  (void*)XamlContainer_AddXamlEvent,  4, {"integer","string","string","pointer"}, "boolean" }
};

static ClassDefinition classDef = {
    "XamlContainer",
    sizeof(XamlContainer),
    (void*)Constructor,
    props, sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &classDef;
}



#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD reason, LPVOID){
    if(reason==DLL_PROCESS_DETACH) Cleanup();
    return TRUE;
}
#endif
} // extern "C"

/* ——— dispatcher ——— */
static void dispatchEvent(int h,const std::string& key,const char* param){
    void* cb=nullptr;
    {
        std::lock_guard<std::mutex> l(gInstMx);
        if(auto it=gInst.find(h); it!=gInst.end()){
            std::lock_guard<std::mutex> l2(it->second->eventsMx);
            if(auto e=it->second->xamlEvents.find(key); e!=it->second->xamlEvents.end())
                cb=e->second;
        }
    }
    if(!cb) return;
    char* dup=_strdup(param?param:"");
    using CB=void(__stdcall*)(const char*);
    ((CB)cb)(dup);
    free(dup);
}
