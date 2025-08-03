/*

  XSharedIPC.cpp
  CrossBasic Plugin: XSharedIPC                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <vector>
#include <string>
#include <cstring>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

using namespace boost::interprocess;

//------------------------------------------------------------------------------
// XSharedIPC Class
//------------------------------------------------------------------------------

class XSharedIPC {
public:
    managed_shared_memory* segment   = nullptr;
    message_queue*        mq        = nullptr;
    named_mutex*          namedMtx  = nullptr;

    XSharedIPC() = default;
    ~XSharedIPC() {
        if (segment)   delete segment;
        if (mq)        delete mq;
        if (namedMtx)  delete namedMtx;
    }

    // Shared memory
    void CreateSharedMemory(const char* name, int size) {
        shared_memory_object::remove(name);
        segment = new managed_shared_memory(create_only, name, size);
    }
    void SetInt(const char* label, int value) {
        if (!segment) return;
        int* slot = segment->construct<int>(label)();
        *slot = value;
    }
    int GetInt(const char* label) {
        if (!segment) return -1;
        auto res = segment->find<int>(label);
        return res.first ? *res.first : -1;
    }
    void SetString(const char* label, const char* value) {
        if (!segment) return;
        size_t len = std::strlen(value) + 1;
        char* slot = segment->construct<char>(label)[len]();
        std::memcpy(slot, value, len);
    }
    const char* GetString(const char* label) {
        if (!segment) return "";
        auto res = segment->find<char>(label);
        return res.first ? res.first : "";
    }
    void CreateIntArray(const char* label, int count) {
        if (!segment) return;
        segment->construct<int>(label)[count]();
    }
    void SetArrayInt(const char* label, int index, int value) {
        if (!segment) return;
        auto res = segment->find<int>(label);
        if (res.first && index >= 0 && index < (int)res.second)
            res.first[index] = value;
    }
    int GetArrayInt(const char* label, int index) {
        if (!segment) return -1;
        auto res = segment->find<int>(label);
        if (res.first && index >= 0 && index < (int)res.second)
            return res.first[index];
        return -1;
    }
    int GetArraySize(const char* label) {
        if (!segment) return 0;
        auto res = segment->find<int>(label);
        return (int)res.second;
    }

    // Message queue
    void CreateQueue(const char* qname, int maxMessages, int messageSize) {
        // remove old pointer
        if (mq) { delete mq; mq = nullptr; }
        // try open_or_create, fall back to open_only
        try {
            mq = new message_queue(open_or_create, qname, maxMessages, messageSize);
        }
        catch (interprocess_exception&) {
            mq = new message_queue(open_only, qname);
        }
    }
    void CloseQueue(const char* qname) {
        if (mq) { delete mq; mq = nullptr; }
        message_queue::remove(qname);
    }
    void SendToQueue(int value) {
        if (mq) mq->send(&value, sizeof(value), 0);
    }
    int ReceiveFromQueue() {
        if (!mq) return -1;
        int val; size_t recvd; unsigned int prio;
        if (mq->try_receive(&val, sizeof(val), recvd, prio))
            return val;
        return -1;
    }
    void SendStringToQueue(const char* text) {
        if (!mq) return;
        size_t len = std::strlen(text) + 1;
        mq->send(text, len, 0);
    }
    const char* ReceiveStringFromQueue() {
        if (!mq) return "";
        static thread_local std::vector<char> buf;
        buf.resize(mq->get_max_msg_size());
        size_t recvd; unsigned int prio;
        if (mq->try_receive(buf.data(), buf.size(), recvd, prio)) {
            buf[recvd - 1] = '\0';
            return buf.data();
        }
        return "";
    }

    // Named mutex
    void CreateNamedMutex(const char* name) {
        if (namedMtx) { delete namedMtx; namedMtx = nullptr; }
        named_mutex::remove(name);
        namedMtx = new named_mutex(open_or_create, name);
    }
    void LockMutex() {
        if (namedMtx) namedMtx->lock();
    }
    void UnlockMutex() {
        if (namedMtx) namedMtx->unlock();
    }
    void RemoveNamedMutex(const char* name) {
        if (namedMtx) { delete namedMtx; namedMtx = nullptr; }
        named_mutex::remove(name);
    }
};

//------------------------------------------------------------------------------
// Global handle management
//------------------------------------------------------------------------------

static std::mutex                           _ipcMutex;
static std::unordered_map<int, XSharedIPC*> _ipcMap;
static std::atomic<int>                     _nextHandle{1};

//------------------------------------------------------------------------------
// C‐exports
//------------------------------------------------------------------------------

extern "C" {

// Constructor / Destructor
XPLUGIN_API int  Constructor()                                   {
    int h = _nextHandle++;
    auto obj = new XSharedIPC();
    std::lock_guard<std::mutex> lk(_ipcMutex);
    _ipcMap[h] = obj;
    return h;
}
XPLUGIN_API void Destructor(int handle)                          {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it = _ipcMap.find(handle); it != _ipcMap.end()) {
        delete it->second;
        _ipcMap.erase(it);
    }
}

// Shared memory
XPLUGIN_API void CreateSharedMemory(int h, const char* name, int size) {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->CreateSharedMemory(name, size);
}
XPLUGIN_API void SetInt(int h, const char* label, int value)     {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->SetInt(label, value);
}
XPLUGIN_API int  GetInt(int h, const char* label)                {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        return it->second->GetInt(label);
    return -1;
}
XPLUGIN_API void SetString(int h, const char* label, const char* v) {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->SetString(label, v);
}
XPLUGIN_API const char* GetString(int h, const char* label)     {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        return it->second->GetString(label);
    return "";
}
XPLUGIN_API void CreateIntArray(int h, const char* label, int cnt) {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->CreateIntArray(label, cnt);
}
XPLUGIN_API void SetArrayInt(int h, const char* label, int idx, int v) {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->SetArrayInt(label, idx, v);
}
XPLUGIN_API int  GetArrayInt(int h, const char* label, int idx)  {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        return it->second->GetArrayInt(label, idx);
    return -1;
}
XPLUGIN_API int  GetArraySize(int h, const char* label)         {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        return it->second->GetArraySize(label);
    return 0;
}

// Message queue
XPLUGIN_API void CreateQueue(int h, const char* q, int m, int s) {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->CreateQueue(q, m, s);
}
XPLUGIN_API void CloseQueue(int h, const char* q)                {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->CloseQueue(q);
}
XPLUGIN_API void SendToQueue(int h, int v)                       {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->SendToQueue(v);
}
XPLUGIN_API int  ReceiveFromQueue(int h)                         {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        return it->second->ReceiveFromQueue();
    return -1;
}
XPLUGIN_API void SendStringToQueue(int h, const char* txt)       {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->SendStringToQueue(txt);
}
XPLUGIN_API const char* ReceiveStringFromQueue(int h)            {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        return it->second->ReceiveStringFromQueue();
    return "";
}

// Named mutex
XPLUGIN_API void CreateNamedMutex(int h, const char* name)       {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->CreateNamedMutex(name);
}
XPLUGIN_API void LockMutex(int h)                                {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->LockMutex();
}
XPLUGIN_API void UnlockMutex(int h)                              {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->UnlockMutex();
}
XPLUGIN_API void RemoveNamedMutex(int h, const char* name)       {
    std::lock_guard<std::mutex> lk(_ipcMutex);
    if (auto it=_ipcMap.find(h); it!=_ipcMap.end())
        it->second->RemoveNamedMutex(name);
}

//------------------------------------------------------------------------------
// ClassDefinition for CrossBasic
//------------------------------------------------------------------------------

typedef struct { const char* name; const char* type; void* getter; void* setter; } ClassProperty;
typedef struct { const char* name; void* funcPtr; int arity; const char* paramTypes[10]; const char* retType; } ClassEntry;
typedef struct { const char* className; size_t classSize; void* constructor; ClassProperty* properties; size_t propertiesCount; ClassEntry* methods; size_t methodsCount; void* constants; size_t constantsCount; } ClassDefinition;

static ClassProperty SharedIPC_Props[] = {};

static ClassEntry SharedIPC_Methods[] = {
    { "CreateSharedMemory",     (void*)CreateSharedMemory,      3, {"integer","string","integer"},             "void" },
    { "SetInt",                 (void*)SetInt,                  3, {"integer","string","integer"},             "void" },
    { "GetInt",                 (void*)GetInt,                  2, {"integer","string"},                       "integer" },
    { "SetString",              (void*)SetString,               3, {"integer","string","string"},              "void" },
    { "GetString",              (void*)GetString,               2, {"integer","string"},                       "string" },
    { "CreateIntArray",         (void*)CreateIntArray,          3, {"integer","string","integer"},             "void" },
    { "SetArrayInt",            (void*)SetArrayInt,             4, {"integer","string","integer","integer"},   "void" },
    { "GetArrayInt",            (void*)GetArrayInt,             3, {"integer","string","integer"},             "integer" },
    { "GetArraySize",           (void*)GetArraySize,            2, {"integer","string"},                       "integer" },
    { "CreateQueue",            (void*)CreateQueue,             4, {"integer","string","integer","integer"},   "void" },
    { "CloseQueue",             (void*)CloseQueue,              2, {"integer","string"},                       "void" },
    { "SendToQueue",            (void*)SendToQueue,             2, {"integer","integer"},                      "void" },
    { "ReceiveFromQueue",       (void*)ReceiveFromQueue,        1, {"integer"},                                "integer" },
    { "SendStringToQueue",      (void*)SendStringToQueue,       2, {"integer","string"},                       "void" },
    { "ReceiveStringFromQueue", (void*)ReceiveStringFromQueue,  1, {"integer"},                                "string" },
    { "CreateNamedMutex",       (void*)CreateNamedMutex,        2, {"integer","string"},                       "void" },
    { "LockMutex",              (void*)LockMutex,               1, {"integer"},                                "void" },
    { "UnlockMutex",            (void*)UnlockMutex,             1, {"integer"},                                "void" },
    { "RemoveNamedMutex",       (void*)RemoveNamedMutex,        2, {"integer","string"},                       "void" },
    { "Close",                   (void*)Destructor,              1, {"integer"},                                "void" }
};

static ClassDefinition SharedIPC_Class = {
    "XSharedIPC",
    sizeof(XSharedIPC),
    (void*)Constructor,
    SharedIPC_Props,
    0,
    SharedIPC_Methods,
    sizeof(SharedIPC_Methods)/sizeof(ClassEntry),
    nullptr,
    0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &SharedIPC_Class;
}

#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif

} // extern "C"
