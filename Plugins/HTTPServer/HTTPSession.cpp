/*

  HttpSession.cpp
  CrossBasic Plugin: HttpSession
 
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
#include <boost/algorithm/string.hpp>
#include <mutex>
#include <unordered_map>
#include <random>
#include <string>
#include <sstream>
#include <map>
#include <memory>
#include <cstring>
#include <cstdlib>

#include <cstdio>
static void say(const char* fmt, ...) {
    va_list ap; va_start(ap,fmt); vprintf(fmt,ap); va_end(ap); fflush(stdout);
}


#define XPLUGIN_API __declspec(dllexport)
#if defined(_MSC_VER)
  #define strdup _strdup
#endif

using boost::asio::ip::tcp;

/* ── imports from HttpResponse / HttpRequest DLLs ────────── */
extern "C" __declspec(dllimport) const char* HttpResponse_ToString(int);
extern "C" {
  __declspec(dllimport) int  HttpRequest_Constructor();
  __declspec(dllimport) void HttpRequest_Method_SET (int,const char*);
  __declspec(dllimport) void HttpRequest_URI_SET    (int,const char*);
  __declspec(dllimport) void HttpRequest_Version_SET(int,const char*);
  __declspec(dllimport) void HttpRequest_Body_SET   (int,const char*);
  __declspec(dllimport) void HttpRequest_SetHeader  (int,const char*,const char*);
}

/* ── event callback registry ─────────────────────────────── */
static std::mutex gEvMx;
static std::unordered_map<int,
        std::unordered_map<std::string,void*>> gCallbacks;

static void fire(int sess,const std::string& ev,const char* param)
{
    void* fp=nullptr;
    { std::lock_guard<std::mutex> lk(gEvMx);
      auto it=gCallbacks.find(sess);
      if(it!=gCallbacks.end()){
          auto jt=it->second.find(ev);
          if(jt!=it->second.end()) fp=jt->second;
      }}
    if(!fp) return;
    using CB = void(__stdcall*)(const char*);
    ((CB)fp)(param?param:"");
}

/* ── SessInst definition ─────────────────────────────────── */
struct SessInst : public std::enable_shared_from_this<SessInst>{
    tcp::socket                sock;
    boost::asio::streambuf     buf;
    int                        handle;
    boost::asio::io_context&   ctx;
    explicit SessInst(boost::asio::io_context& c):sock(c),ctx(c){}

    void start(){ doRead(); }

private:
void doRead()
{
    auto self = shared_from_this();
    boost::asio::async_read_until(sock, buf, "\r\n\r\n",
        [this,self](boost::system::error_code ec, std::size_t)
    {
        if (ec)
        {
            say("‼ read_until error %d (%s)\n",
                ec.value(), ec.message().c_str());
            return;
        }

        /* ---------- SUCCESS PATH (restored) ---------- */
        std::istream is(&buf);
        std::string  raw;
        std::getline(is, raw, '\0');

        int reqH = parseRequest(raw);
        fire(handle, "request",
             std::to_string(reqH).c_str());
        /* --------------------------------------------- */
    });
}

    int parseRequest(const std::string& raw)
    {
        int h=HttpRequest_Constructor();
        std::istringstream ss(raw);
        std::string m,u,v,line; ss>>m>>u>>v;
        HttpRequest_Method_SET (h,m.c_str());
        HttpRequest_URI_SET    (h,u.c_str());
        HttpRequest_Version_SET(h,v.c_str());
        std::getline(ss,line);

        while(std::getline(ss,line)&&!line.empty()&&line!="\r"){
            auto p=line.find(':');
            if(p!=std::string::npos){
                std::string k=line.substr(0,p);
                std::string val=line.substr(p+1);
                boost::algorithm::trim(k);
                boost::algorithm::trim(val);
                HttpRequest_SetHeader(h,k.c_str(),val.c_str());
            }
        }
        std::string body; std::getline(ss,body,'\0');
        HttpRequest_Body_SET(h,body.c_str());
        return h;
    }

public:
    void sendResponse(int respH)
    {
        const char* s=HttpResponse_ToString(respH);
        auto out=std::make_shared<std::string>(s);
        free((void*)s);
        auto self=shared_from_this();
        boost::asio::async_write(sock,boost::asio::buffer(*out),
            [self,out](boost::system::error_code, std::size_t){});
    }
};

/* ── global registry  {handle → SessInst} ─────────────────── */
static std::mutex                                     gMx;
static std::unordered_map<int,std::shared_ptr<SessInst>> gInst;
static std::mt19937                                   gRnd{std::random_device{}()};
static std::uniform_int_distribution<int>             gDist(10000000,99999999);

/* ── exported factory helpers (used by HttpServer) ───────── */
extern "C" {

XPLUGIN_API std::shared_ptr<SessInst>
makeSession(boost::asio::io_context& ctx, tcp::socket&& sock)
{
    auto s=std::make_shared<SessInst>(ctx);
    s->sock=std::move(sock);
    return s;
}

XPLUGIN_API int HttpSession_Register(std::shared_ptr<SessInst> s)
{
    std::lock_guard<std::mutex> lk(gMx);
    int h; do{h=gDist(gRnd);}while(gInst.count(h));
    s->handle=h; gInst[h]=std::move(s); return h;
}

XPLUGIN_API void HttpSession_Begin(int h)
{
    std::shared_ptr<SessInst> s;
    { std::lock_guard<std::mutex> lk(gMx);
      auto it=gInst.find(h); if(it==gInst.end()) return; s=it->second; }
    s->start();
}

/* ── public API ──────────────────────────────────────────── */
XPLUGIN_API void HttpSession_SendResponse(int h,int respH)
{
    std::shared_ptr<SessInst> s;
    { std::lock_guard<std::mutex> lk(gMx);
      auto it=gInst.find(h); if(it==gInst.end()) return; s=it->second; }
    s->sendResponse(respH);
}

/* helper renamed to avoid CRT dup(int) clash */
static char* dupstr(const std::string& s){ return strdup(s.c_str()); }

XPLUGIN_API const char* HttpSession_Request_GET(int h)
{
    return dupstr("httpsession:"+std::to_string(h)+":request");
}

XPLUGIN_API bool HttpSession_SetEventCallback(int h,const char* tok,void* fp)
{
    { std::lock_guard<std::mutex> lk(gMx); if(!gInst.count(h)) return false; }
    std::string key = tok?tok:"";
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
  {"Request","string",(void*)HttpSession_Request_GET,nullptr}
};
static ClassMeth kMeths[]={
  {"SendResponse",(void*)HttpSession_SendResponse   ,2,{"integer","HttpResponse"},"void"},
  {"HTTPSession_SetEventCallback",(void*)HttpSession_SetEventCallback,3,{"integer","string","pointer"},"boolean"}
};
static ClassDefinition gDef={
  "HttpSession",
  sizeof(SessInst),
  nullptr,
  kProps,sizeof(kProps)/sizeof(kProps[0]),
  kMeths,sizeof(kMeths)/sizeof(kMeths[0]),
  nullptr,0
};
XPLUGIN_API ClassDefinition* GetClassDefinition(){return &gDef;}

/* ── cleanup ─────────────────────────────────────────────── */
static void CleanupAll(){ std::lock_guard<std::mutex> lk(gMx); gInst.clear(); }
BOOL APIENTRY DllMain(HMODULE,DWORD r,LPVOID){
  if(r==DLL_PROCESS_DETACH) CleanupAll(); return TRUE; }

} /* extern "C" */
