/*

  HttpResponse.cpp
  CrossBasic Plugin: HttpResponse                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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

#include <string>
#include <map>
#include <mutex>
#include <unordered_map>
#include <random>
#include <sstream>
#include <cstring>
#include <cstdlib>

#define XPLUGIN_API __declspec(dllexport)
#if defined(_MSC_VER)
  #define strdup _strdup
#endif

/* ── instance ─────────────────────────────────────────────── */
struct HttpRespInst{
    std::string                       statusCode="200",statusMsg="OK",body;
    std::map<std::string,std::string> headers;
};

/* ── registry ─────────────────────────────────────────────── */
static std::mutex                             gMx;
static std::unordered_map<int,HttpRespInst*>  gInst;
static std::mt19937                           gRnd{std::random_device{}()};
static std::uniform_int_distribution<int>     gDist(10000000,99999999);

/* ── ctor/dtor ────────────────────────────────────────────── */
extern "C" {

XPLUGIN_API int HttpResponse_Constructor(){
    std::lock_guard<std::mutex> lk(gMx);
    int h; do{h=gDist(gRnd);}while(gInst.count(h));
    gInst[h]=new HttpRespInst; return h;
}
XPLUGIN_API void HttpResponse_Close(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h); if(it!=gInst.end()){delete it->second;gInst.erase(it);}
}

/* ── properties ───────────────────────────────────────────── */
static char* dupstr(const std::string& s){ return strdup(s.c_str()); }
#define STR_PROP(N,F)                                                           \
  XPLUGIN_API const char* HttpResponse_##N##_GET(int h){                        \
      std::lock_guard<std::mutex> lk(gMx);                                      \
      auto it=gInst.find(h); return it!=gInst.end()?dupstr(it->second->F):strdup("");}\
  XPLUGIN_API void HttpResponse_##N##_SET(int h,const char* v){                 \
      std::lock_guard<std::mutex> lk(gMx);                                      \
      auto it=gInst.find(h); if(it!=gInst.end()) it->second->F = v?v:"";}

STR_PROP(StatusCode   , statusCode)
STR_PROP(StatusMessage, statusMsg )
STR_PROP(Body         , body      )

/* ── headers ──────────────────────────────────────────────── */
XPLUGIN_API void HttpResponse_SetHeader(int h,const char* k,const char* v){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h); if(it==gInst.end()) return;
    it->second->headers[k?k:""]=v?v:"";
}
XPLUGIN_API const char* HttpResponse_GetHeader(int h,const char* k){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h); if(it==gInst.end()) return strdup("");
    auto jt=it->second->headers.find(k?k:"");
    return jt!=it->second->headers.end()?dupstr(jt->second):strdup("");
}

/* ── serialize ────────────────────────────────────────────── */
static std::string serialize(HttpRespInst* r){
    std::ostringstream ss;
    ss<<"HTTP/1.1 "<<r->statusCode<<' '<<r->statusMsg<<"\r\n";
    if(!r->headers.count("Content-Length"))
        r->headers["Content-Length"]=std::to_string(r->body.size());
    for(auto& h:r->headers) ss<<h.first<<": "<<h.second<<"\r\n";
    ss<<"\r\n"<<r->body; return ss.str();
}
XPLUGIN_API const char* HttpResponse_ToString(int h){
    std::lock_guard<std::mutex> lk(gMx);
    auto it=gInst.find(h); if(it==gInst.end()) return strdup("");
    return dupstr(serialize(it->second));
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
 {"StatusCode","string",(void*)HttpResponse_StatusCode_GET,(void*)HttpResponse_StatusCode_SET},
 {"StatusMessage","string",(void*)HttpResponse_StatusMessage_GET,(void*)HttpResponse_StatusMessage_SET},
 {"Body","string",(void*)HttpResponse_Body_GET,(void*)HttpResponse_Body_SET}
};
static ClassMeth kMeths[]={
 {"SetHeader",(void*)HttpResponse_SetHeader,3,{"integer","string","string"},"void"},
 {"GetHeader",(void*)HttpResponse_GetHeader,2,{"integer","string"},"string"},
 {"ToString",(void*)HttpResponse_ToString ,1,{"integer"},"string"}
};
static ClassDefinition gDef={
 "HttpResponse",
 sizeof(HttpRespInst),
 (void*)HttpResponse_Constructor,
 kProps,sizeof(kProps)/sizeof(kProps[0]),
 kMeths,sizeof(kMeths)/sizeof(kMeths[0]),
 nullptr,0
};
XPLUGIN_API ClassDefinition* GetClassDefinition(){return &gDef;}

/* ── cleanup ─────────────────────────────────────────────── */
static void CleanupAll(){ std::lock_guard<std::mutex> lk(gMx);
  for(auto&kv:gInst) delete kv.second; gInst.clear(); }
BOOL APIENTRY DllMain(HMODULE,DWORD r,LPVOID){
  if(r==DLL_PROCESS_DETACH) CleanupAll(); return TRUE; }

} /* extern "C" */
