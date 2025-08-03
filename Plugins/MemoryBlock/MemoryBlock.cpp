/*

  MemoryBlock.cpp
  CrossBasic Plugin: MemoryBlock                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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

#include <vector>
#include <cstring>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <string>      

#ifdef _WIN32
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// ─────────────────────────────────────────────────────────────────────────────
// MemoryBlock – internal class
// ─────────────────────────────────────────────────────────────────────────────
class MemoryBlock {
public:
    int               Handle;
    std::vector<char> block;

    MemoryBlock() : Handle(0) {}

    // Resize the memory block.
    void Resize(int newSize) {
        if (newSize < 0) return;
        block.resize(static_cast<size_t>(newSize));
    }

    // Basic info
    int Size() const { return static_cast<int>(block.size()); }

    // ── Reads ────────────────────────────────────────────────────────────────
    int    ReadByte  (int o) const;
    int    ReadShort (int o) const;
    int    ReadLong  (int o) const;
    double ReadDouble(int o) const;
    const char* ReadString(int o, int len) const;

    // ── Writes ───────────────────────────────────────────────────────────────
    void WriteByte  (int o, int v);
    void WriteShort (int o, int v);
    void WriteLong  (int o, int v);
    void WriteDouble(int o, double v);
    void WriteString(int o, const char* v);

    // ── Bulk copy ────────────────────────────────────────────────────────────
    void CopyData(int destOffset, MemoryBlock* src, int srcOffset, int len);
};

// ── Read helpers ─────────────────────────────────────────────────────────────
inline int MemoryBlock::ReadByte(int o) const {
    return (o < 0 || o >= Size()) ? -1 : static_cast<unsigned char>(block[static_cast<size_t>(o)]);
}

inline int MemoryBlock::ReadShort(int o) const {
    if (o < 0 || o + 1 >= Size()) return -1;
    short v; std::memcpy(&v, &block[static_cast<size_t>(o)], sizeof(v));
    return v;
}

inline int MemoryBlock::ReadLong(int o) const {
    if (o < 0 || o + 3 >= Size()) return -1;
    int v; std::memcpy(&v, &block[static_cast<size_t>(o)], sizeof(v));
    return v;
}

inline double MemoryBlock::ReadDouble(int o) const {
    if (o < 0 || o + 7 >= Size()) return -1.0;
    double v; std::memcpy(&v, &block[static_cast<size_t>(o)], sizeof(v));
    return v;
}

inline const char* MemoryBlock::ReadString(int o, int len) const {
    static thread_local std::string buf;
    if (o < 0 || len < 0 || o + len > Size()) return "";
    buf.assign(block.begin() + o, block.begin() + o + len);
    return buf.c_str();
}

// ── Write helpers ────────────────────────────────────────────────────────────
inline void MemoryBlock::WriteByte(int o, int v) {
    if (o < 0 || o >= Size()) return;
    block[static_cast<size_t>(o)] = static_cast<char>(v);
}

inline void MemoryBlock::WriteShort(int o, int v) {
    if (o < 0 || o + 1 >= Size()) return;
    short s = static_cast<short>(v);
    std::memcpy(&block[static_cast<size_t>(o)], &s, sizeof(s));
}

inline void MemoryBlock::WriteLong(int o, int v) {
    if (o < 0 || o + 3 >= Size()) return;
    std::memcpy(&block[static_cast<size_t>(o)], &v, sizeof(v));
}

inline void MemoryBlock::WriteDouble(int o, double v) {
    if (o < 0 || o + 7 >= Size()) return;
    std::memcpy(&block[static_cast<size_t>(o)], &v, sizeof(v));
}

inline void MemoryBlock::WriteString(int o, const char* v) {
    if (!v) return;
    int len = static_cast<int>(std::strlen(v));
    if (o < 0 || o + len > Size()) return;
    std::memcpy(&block[static_cast<size_t>(o)], v, static_cast<size_t>(len));
}

// ── Bulk copy ────────────────────────────────────────────────────────────────
inline void MemoryBlock::CopyData(int destOffset, MemoryBlock* src, int srcOffset, int len) {
    if (!src) return;
    if (destOffset < 0 || srcOffset < 0) return;
    if (destOffset + len > Size() || srcOffset + len > src->Size()) return;
    std::memcpy(&block[static_cast<size_t>(destOffset)],
                &src->block[static_cast<size_t>(srcOffset)],
                static_cast<size_t>(len));
}

// ─────────────────────────────────────────────────────────────────────────────
// Handle ↔ instance bookkeeping
// ─────────────────────────────────────────────────────────────────────────────
static std::mutex                               memMtx;
static std::unordered_map<int, MemoryBlock*>    blocks;
static std::atomic<int>                         nextHandle(1);

static inline MemoryBlock* fetch(int h) {
    auto it = blocks.find(h);
    return (it == blocks.end()) ? nullptr : it->second;
}

// ─────────────────────────────────────────────────────────────────────────────
// C-linkage API – exported to the VM
// ─────────────────────────────────────────────────────────────────────────────
extern "C" {

// Constructor
XPLUGIN_API int NewMemoryBlock() {
    int h = nextHandle.fetch_add(1);
    auto* b = new MemoryBlock();
    b->Handle = h;
    std::lock_guard<std::mutex> lk(memMtx);
    blocks[h] = b;
    return h;
}

// Handle (read-only)
XPLUGIN_API int MemoryBlock_GetHandle(int h) { return h; }

// Size
XPLUGIN_API int MemoryBlock_Size(int h) {
    std::lock_guard<std::mutex> lk(memMtx);
    if (auto* p = fetch(h)) return p->Size();
    return -1;
}

// Resize
XPLUGIN_API void MemoryBlock_Resize(int h, int newSize) {
    std::lock_guard<std::mutex> lk(memMtx);
    if (auto* p = fetch(h)) p->Resize(newSize);
}

// Destroy
XPLUGIN_API bool MemoryBlock_Destroy(int h) {
    std::lock_guard<std::mutex> lk(memMtx);
    auto it = blocks.find(h);
    if (it == blocks.end()) return false;
    delete it->second;
    blocks.erase(it);
    return true;
}

// ── Reads ───────────────────────────────────────────────────────────────────
XPLUGIN_API int    MemoryBlock_ReadByte  (int h, int o) { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); return p ? p->ReadByte(o)   : -1; }
XPLUGIN_API int    MemoryBlock_ReadShort (int h, int o) { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); return p ? p->ReadShort(o)  : -1; }
XPLUGIN_API int    MemoryBlock_ReadLong  (int h, int o) { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); return p ? p->ReadLong(o)   : -1; }
XPLUGIN_API double MemoryBlock_ReadDouble(int h, int o) { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); return p ? p->ReadDouble(o) : -1.0; }
XPLUGIN_API const char* MemoryBlock_ReadString(int h, int o, int l) {
    std::lock_guard<std::mutex> lk(memMtx);
    auto* p = fetch(h);
    return p ? p->ReadString(o, l) : "";
}

// ── Writes ──────────────────────────────────────────────────────────────────
XPLUGIN_API void MemoryBlock_WriteByte  (int h, int o, int v)       { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); if (p) p->WriteByte(o, v); }
XPLUGIN_API void MemoryBlock_WriteShort (int h, int o, int v)       { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); if (p) p->WriteShort(o, v); }
XPLUGIN_API void MemoryBlock_WriteLong  (int h, int o, int v)       { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); if (p) p->WriteLong(o, v); }
XPLUGIN_API void MemoryBlock_WriteDouble(int h, int o, double v)    { std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); if (p) p->WriteDouble(o, v); }
XPLUGIN_API void MemoryBlock_WriteString(int h, int o, const char* s){ std::lock_guard<std::mutex> lk(memMtx); auto* p = fetch(h); if (p) p->WriteString(o, s); }

// CopyData
XPLUGIN_API void MemoryBlock_CopyData(int destH, int destOff, int srcH, int srcOff, int len) {
    std::lock_guard<std::mutex> lk(memMtx);
    auto* dest = fetch(destH);
    auto* src  = fetch(srcH);
    if (dest && src) dest->CopyData(destOff, src, srcOff, len);
}

} // extern "C"

// ─────────────────────────────────────────────────────────────────────────────
// Class definition tables
// ─────────────────────────────────────────────────────────────────────────────
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

// Properties
static ClassProperty props[] = {
    { "Handle", "integer", (void*)MemoryBlock_GetHandle, nullptr },
    { "Size",   "integer", (void*)MemoryBlock_Size,      nullptr }
};

// Methods
static ClassEntry methods[] = {
    { "Resize",     (void*)MemoryBlock_Resize,     2, {"integer","integer"},                            "void"    },
    { "Close",      (void*)MemoryBlock_Destroy,    1, {"integer"},                                      "boolean" },
    { "ReadByte",   (void*)MemoryBlock_ReadByte,   2, {"integer","integer"},                            "integer" },
    { "ReadShort",  (void*)MemoryBlock_ReadShort,  2, {"integer","integer"},                            "integer" },
    { "ReadLong",   (void*)MemoryBlock_ReadLong,   2, {"integer","integer"},                            "integer" },
    { "ReadDouble", (void*)MemoryBlock_ReadDouble, 2, {"integer","integer"},                            "double"  },
    { "ReadString", (void*)MemoryBlock_ReadString, 3, {"integer","integer","integer"},                  "string"  },
    { "WriteByte",  (void*)MemoryBlock_WriteByte,  3, {"integer","integer","integer"},                  "void"    },
    { "WriteShort", (void*)MemoryBlock_WriteShort, 3, {"integer","integer","integer"},                  "void"    },
    { "WriteLong",  (void*)MemoryBlock_WriteLong,  3, {"integer","integer","integer"},                  "void"    },
    { "WriteDouble",(void*)MemoryBlock_WriteDouble,3, {"integer","integer","double"},                   "void"    },
    { "WriteString",(void*)MemoryBlock_WriteString,3, {"integer","integer","string"},                   "void"    },
    { "CopyData",   (void*)MemoryBlock_CopyData,   5, {"integer","integer","MemoryBlock","integer","integer"}, "void" }
};

// No constants
static ClassConstant constants[] = {};

// Class definition
static ClassDefinition classDef = {
    "MemoryBlock",
    sizeof(MemoryBlock),
    (void*)NewMemoryBlock,
    props,   sizeof(props)   / sizeof(ClassProperty),
    methods, sizeof(methods) / sizeof(ClassEntry),
    constants, sizeof(constants) / sizeof(ClassConstant)
};

// Export definition
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() { return &classDef; }

// Windows DLL entry
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif
