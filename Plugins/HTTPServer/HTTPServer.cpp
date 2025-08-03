/*

  HTTPServer.cpp
  CrossBasic Plugin: HTTPServer
 
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

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
#endif

#include <boost/asio.hpp>
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <random>
#include <memory>
#include <cstring>

#include <cstdio>
static void say(const char* fmt, ...) {
    va_list ap; va_start(ap,fmt); vprintf(fmt,ap); va_end(ap); fflush(stdout);
}


#define XPLUGIN_API __declspec(dllexport)
#if defined(_MSC_VER)
  #define strdup _strdup
#endif

using boost::asio::ip::tcp;

/* ── imports from HTTPSession.dll ─────────────────────────── */
extern "C" {
  __declspec(dllimport) std::shared_ptr<struct SessInst>
    makeSession(boost::asio::io_context&, tcp::socket&&);
  __declspec(dllimport) int  HttpSession_Register(std::shared_ptr<SessInst>);
  __declspec(dllimport) void HttpSession_Begin(int);
}

/* ── event table (serverHandle → event → fnPtr) ───────────── */
static std::mutex gEvMx;
static std::unordered_map<int,
        std::unordered_map<std::string,void*>> gCallbacks;

static void fire(int srv, const std::string& ev, const char* param)
{
    void* fp=nullptr;
    { std::lock_guard<std::mutex> lk(gEvMx);
      auto it=gCallbacks.find(srv);
      if(it!=gCallbacks.end()){
          auto jt=it->second.find(ev);
          if(jt!=it->second.end()) fp=jt->second;
      }}
    if(!fp) return;
    using CB=void(__stdcall*)(const char*);
    ((CB)fp)(param?param:"");
}

/* ── ServInst ─────────────────────────────────────────────── */
struct ServInst{
    int                             handle;
    unsigned short                  port;
    boost::asio::io_context         ctx;
    std::unique_ptr<tcp::acceptor>  acceptor;
    std::vector<std::thread>        threads;
    bool                            running=false;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>guard;
    /* ServInst constructor */
    explicit ServInst(unsigned short p)
    : port(p), guard(ctx.get_executor()) {}   // ← keeps ctx alive
};

/* ── global registry ──────────────────────────────────────── */
static std::mutex                        gMx;
static std::unordered_map<int,ServInst*> gInst;
static std::mt19937                      gRnd{std::random_device{}()};
static std::uniform_int_distribution<int>gDist(10000000,99999999);

/* ── accept loop ──────────────────────────────────────────── */
static void doAccept(ServInst* si)
{
    auto sock = std::make_shared<tcp::socket>(si->ctx);

    si->acceptor->async_accept(*sock,
        [si,sock](boost::system::error_code ec)
    {
        if (ec)
        {
            say("‼ HttpServer accept error %d (%s)\n",
                ec.value(), ec.message().c_str());

            /* Retry after one second */
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (si->running) doAccept(si);
            return;
        }

        /* ---------- SUCCESS PATH (was commented out) ---------- */
        auto sPtr  = makeSession(si->ctx, std::move(*sock));
        int sessH  = HttpSession_Register(sPtr);
        HttpSession_Begin(sessH);

        fire(si->handle, "session",
             std::to_string(sessH).c_str());
        /* ------------------------------------------------------ */

        if (si->running) doAccept(si);
    });
}



/* ── ctor / dtor ──────────────────────────────────────────── */
extern "C" {

XPLUGIN_API int HttpServer_Constructor(int port)
{
    auto si=new ServInst(static_cast<unsigned short>(port));
    {
        std::lock_guard<std::mutex> lk(gMx);
        int h; do{h=gDist(gRnd);}while(gInst.count(h));
        si->handle=h; gInst[h]=si;
    }
    return si->handle;
}
XPLUGIN_API void HttpServer_Close(int h)
{
    ServInst* si;
    { std::lock_guard<std::mutex> lk(gMx);
      auto it=gInst.find(h); if(it==gInst.end()) return; si=it->second; gInst.erase(it);}
    si->running=false;
    si->ctx.stop();
    for(auto& t:si->threads) if(t.joinable()) t.join();
    delete si;
}

/* ── control ──────────────────────────────────────────────── */
XPLUGIN_API void HttpServer_Start(int h)
{
    ServInst* si;
    { std::lock_guard<std::mutex> lk(gMx);
      auto it=gInst.find(h); if(it==gInst.end()) return; si=it->second; }
    if(si->running) return;

    si->acceptor=std::make_unique<tcp::acceptor>(
                    si->ctx, tcp::endpoint(tcp::v4(),si->port));
    si->running=true;
    doAccept(si);

    unsigned n=std::max(1u,std::thread::hardware_concurrency());
    for(unsigned i=0;i<n;++i)
        si->threads.emplace_back([si](){ si->ctx.run(); });
}
XPLUGIN_API void HttpServer_Stop(int h){ HttpServer_Close(h); }

/* ── helpers ──────────────────────────────────────────────── */
static char* dupstr(const std::string& s){ return strdup(s.c_str()); }

XPLUGIN_API const char* HttpServer_Session_GET(int h)
{
    return dupstr("httpserver:"+std::to_string(h)+":session");
}
XPLUGIN_API int HttpServer_Port_GET(int h)
{
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h); return it!=gInst.end()?it->second->port:0;
}

/* ── event-callback setter ───────────────────────────────── */
XPLUGIN_API bool HttpServer_SetEventCallback(int h,const char* tok,void* fp)
{
    { std::lock_guard<std::mutex> lk(gMx); if(!gInst.count(h)) return false; }
    std::string key=tok?tok:"";
    auto p=key.rfind(':'); if(p!=std::string::npos) key.erase(0,p+1);
    std::lock_guard<std::mutex> lk2(gEvMx);
    gCallbacks[h][key]=fp; return true;
}

/* ── CrossBasic registration ─────────────────────────────── */
typedef struct{const char* n;const char* t;void* g;void* s;} ClassProp;
typedef struct{const char* n;void* fp;int a;const char* pt[10];const char* rt;} ClassMeth;
typedef struct{
  const char* className; size_t classSize; void* ctor;
  ClassProp* props; size_t propsCnt;
  ClassMeth* meths; size_t methCnt;
  void* consts; size_t constCnt;
}ClassDefinition;

static ClassProp kProps[]={
  {"Port"   , "integer", (void*)HttpServer_Port_GET   , nullptr},
  {"Session", "string" , (void*)HttpServer_Session_GET, nullptr}
};
static ClassMeth kMeths[]={
  {"Start"           ,(void*)HttpServer_Start          ,1,{"integer"},"void"},
  {"Stop"            ,(void*)HttpServer_Stop           ,1,{"integer"},"void"},
  {"HTTPServer_SetEventCallback",(void*)HttpServer_SetEventCallback,3,{"integer","string","pointer"},"boolean"}
};
static ClassDefinition gDef={
  "HttpServer",
  sizeof(ServInst),
  (void*)HttpServer_Constructor,
  kProps,sizeof(kProps)/sizeof(kProps[0]),
  kMeths,sizeof(kMeths)/sizeof(kMeths[0]),
  nullptr,0
};
XPLUGIN_API ClassDefinition* GetClassDefinition(){return &gDef;}

/* ── cleanup ─────────────────────────────────────────────── */
static void CleanupAll(){
    std::lock_guard<std::mutex> lk(gMx);
    std::vector<int> ids; for(auto&kv:gInst) ids.push_back(kv.first);
    for(int id:ids) HttpServer_Close(id);
}
BOOL APIENTRY DllMain(HMODULE,DWORD r,LPVOID){
  if(r==DLL_PROCESS_DETACH) CleanupAll(); return TRUE; }

} /* extern "C" */
