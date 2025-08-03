/*

  JSONItem.cpp
  CrossBasic Plugin: JSONItem                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #define WIN32_LEAN_AND_MEAN
  #define _WINSOCKAPI_
  #include <windows.h>
  #undef byte
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <mutex>
#include <map>
#include <atomic>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

// ─────────────────────────────────────────────────────────────────────────────
// helper: best-effort parse – returns {parsed = true} when legal JSON,
// otherwise {parsed = false, j = unused}
// ─────────────────────────────────────────────────────────────────────────────
static json tryParse(const string& src, bool& parsed) noexcept {
    try {
        json j = json::parse(src);
        parsed = true;
        return j;
    } catch (...) {
        parsed = false;
        return json();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// JSONItem – internal class
// ─────────────────────────────────────────────────────────────────────────────
class JSONItem {
public:
    int   Handle;
    json  data;
    bool  Compact;
    int   IndentSpacing;

    JSONItem()
      : Handle(0),
        data(json::object()),
        Compact(true),
        IndentSpacing(3)
    {}

    // basic I/O
    void   Load(const string& s)        { data = json::parse(s); }
    string ToString() const             { return Compact ? data.dump()
                                                      : data.dump(IndentSpacing); }

    // structure queries
    int    Count() const                { return data.is_object() || data.is_array()
                                             ? (int)data.size() : 0; }
    bool   HasKey(const string& k) const{ return data.is_object() && data.contains(k); }
    bool   IsArray() const              { return data.is_array(); }

    vector<string> Keys() const {
        vector<string> k;
        if (data.is_object())
            for (auto& e : data.items())
                k.push_back(e.key());
        return k;
    }

    int LastRowIndex() const            { return data.is_array()
                                             ? (int)data.size() - 1
                                             : -1; }

    string Lookup(const string& k, const string& def) const {
        return HasKey(k) ? data[k].dump() : def;
    }

    void Remove(const string& k)        { if (data.is_object()) data.erase(k); }
    void RemoveAll()                    { data.clear(); }

    // ── Array helpers ───────────────────────────────────────────────────────
    void Add(const string& v) {
        if (!data.is_array()) data = json::array();
        bool ok = false;
        json j = tryParse(v, ok);
        data.push_back(ok ? j : json(v));
    }

    void AddAt(int idx, const string& v) {
        if (!data.is_array()) data = json::array();
        if (idx < 0 || idx > (int)data.size()) return;
        bool ok = false;
        json j = tryParse(v, ok);
        data.insert(data.begin() + idx, ok ? j : json(v));
    }

    string ValueAt(int idx) const {
        if (!data.is_array() || idx < 0 || idx >= (int)data.size())
            throw out_of_range("ValueAt – index out of range");
        return data[idx].dump();
    }

    void RemoveAt(int idx) {
        if (data.is_array() && idx >= 0 && idx < (int)data.size())
            data.erase(data.begin() + idx);
    }

    // ── Object helpers ──────────────────────────────────────────────────────
    string Value(const string& k) const {
        return HasKey(k) ? data[k].dump() : "";
    }

    void SetValue(const string& k, const string& v) {
        if (!data.is_object()) data = json::object();
        bool ok = false;
        json j = tryParse(v, ok);
        data[k] = ok ? j : json(v);
    }

    // ── Child JSONItem helpers ─────────────────────────────────────────────
    JSONItem Child(const string& k) const {
        JSONItem c;
        if (HasKey(k)) c.data = data[k];
        return c;
    }

    void SetChild(const string& k, const JSONItem& child) {
        if (!data.is_object()) data = json::object();
        data[k] = child.data;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// handle ↔ instance management
// ─────────────────────────────────────────────────────────────────────────────
static mutex               mtx;
static map<int, JSONItem*> instances;
static atomic<int>         nextHandle(1);

static JSONItem* fetch(int h) {
    auto it = instances.find(h);
    return it == instances.end() ? nullptr : it->second;
}

static int makeHandle(JSONItem* p) {
    int h = nextHandle.fetch_add(1);
    p->Handle = h;
    instances[h] = p;
    return h;
}

// ─────────────────────────────────────────────────────────────────────────────
// C-linkage API – exported to the byte-code VM
// ─────────────────────────────────────────────────────────────────────────────
extern "C" {

// Constructor
XPLUGIN_API int NewJSONItem() {
    lock_guard<mutex> lk(mtx);
    return makeHandle(new JSONItem());
}

// ToString property
XPLUGIN_API const char* JSONItem_ToString(int h) {
    static string out;
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) out = p->ToString();
    else                   out.clear();
    return out.c_str();
}

// Count property (getter only)
XPLUGIN_API int JSONItem_Count(int h) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) return p->Count();
    return 0;
}

// IsArray
XPLUGIN_API bool JSONItem_IsArray(int h) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) return p->IsArray();
    return false;
}

// HasKey
XPLUGIN_API bool JSONItem_HasKey(int h, const char* k) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) return p->HasKey(k);
    return false;
}

// LastRowIndex
XPLUGIN_API int JSONItem_LastRowIndex(int h) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) return p->LastRowIndex();
    return -1;
}

// Handle getter
XPLUGIN_API int JSONItem_GetHandle(int h) { return h; }

// Compact property
XPLUGIN_API const char* JSONItem_GetCompact(int h) {
    static string s;
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) s = p->Compact ? "true" : "false";
    else                   s = "false";
    return s.c_str();
}
XPLUGIN_API void JSONItem_SetCompact(int h, bool v) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->Compact = v;
}

// IndentSpacing property
XPLUGIN_API int  JSONItem_GetIndentSpacing(int h) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) return p->IndentSpacing;
    return 0;
}
XPLUGIN_API void JSONItem_SetIndentSpacing(int h, int v) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->IndentSpacing = v;
}

// Load
XPLUGIN_API const char* JSONItem_Load(int h, const char* j) {
    static string res;
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) {
        try { p->Load(j); res = "OK"; }
        catch (const exception& e) { res = e.what(); }
    }
    else res = "Error: invalid handle";
    return res.c_str();
}

// Lookup
XPLUGIN_API const char* JSONItem_Lookup(int h, const char* k, const char* d) {
    static string out;
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) out = p->Lookup(k, d);
    else                   out = d;
    return out.c_str();
}

// Value
XPLUGIN_API const char* JSONItem_Value(int h, const char* k) {
    static string out;
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) out = p->Value(k);
    else                   out.clear();
    return out.c_str();
}

// SetValue
XPLUGIN_API void JSONItem_SetValue(int h, const char* k, const char* v) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->SetValue(k, v);
}

// Keys
XPLUGIN_API const char* JSONItem_Keys(int h) {
    static string out;
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) out = json(p->Keys()).dump();
    else                   out = "[]";
    return out.c_str();
}

// Remove
XPLUGIN_API void JSONItem_Remove(int h, const char* k) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->Remove(k);
}

// RemoveAll
XPLUGIN_API void JSONItem_RemoveAll(int h) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->RemoveAll();
}

// Add
XPLUGIN_API void JSONItem_Add(int h, const char* v) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->Add(v);
}

// AddAt
XPLUGIN_API void JSONItem_AddAt(int h, int i, const char* v) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->AddAt(i, v);
}

// ValueAt
XPLUGIN_API const char* JSONItem_ValueAt(int h, int i) {
    static string out;
    lock_guard<mutex> lk(mtx);
    try {
        if (auto p = fetch(h)) out = p->ValueAt(i);
    } catch (const exception& e) {
        out = e.what();
    }
    return out.c_str();
}

// RemoveAt
XPLUGIN_API void JSONItem_RemoveAt(int h, int i) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) p->RemoveAt(i);
}

// Child
XPLUGIN_API int JSONItem_Child(int h, const char* k) {
    lock_guard<mutex> lk(mtx);
    if (auto p = fetch(h)) {
        JSONItem* c = new JSONItem(p->Child(k));
        return makeHandle(c);
    }
    return 0;
}

// SetChild
XPLUGIN_API void JSONItem_SetChild(int h, const char* k, int childHandle) {
    lock_guard<mutex> lk(mtx);
    auto parent = fetch(h);
    auto child  = fetch(childHandle);
    if (parent && child) parent->SetChild(k, *child);
}

// Destroy
XPLUGIN_API bool JSONItem_Destroy(int h) {
    lock_guard<mutex> lk(mtx);
    auto it = instances.find(h);
    if (it == instances.end()) return false;
    delete it->second;
    instances.erase(it);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Class definition table
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

static ClassProperty props[] = {
    { "Compact",       "boolean", (void*)JSONItem_GetCompact,       (void*)JSONItem_SetCompact },
    { "IndentSpacing", "integer", (void*)JSONItem_GetIndentSpacing, (void*)JSONItem_SetIndentSpacing },
    { "ToString",      "string",  (void*)JSONItem_ToString,         nullptr },
    { "Handle",        "integer", (void*)JSONItem_GetHandle,        nullptr },
    { "Count",         "integer", (void*)JSONItem_Count,            nullptr }
};

static ClassEntry methods[] = {
    { "Load",        (void*)JSONItem_Load,       2, {"integer","string"},           "string"    },
    { "HasKey",      (void*)JSONItem_HasKey,     2, {"integer","string"},           "boolean"   },
    { "IsArray",     (void*)JSONItem_IsArray,    1, {"integer"},                    "boolean"   },
    { "Keys",        (void*)JSONItem_Keys,       1, {"integer"},                    "string"    },
    { "LastRowIndex",(void*)JSONItem_LastRowIndex,1,{"integer"},                    "integer"   },
    { "Lookup",      (void*)JSONItem_Lookup,     3, {"integer","string","string"},  "string"    },
    { "Remove",      (void*)JSONItem_Remove,     2, {"integer","string"},           "void"      },
    { "RemoveAll",   (void*)JSONItem_RemoveAll,  1, {"integer"},                    "void"      },
    { "Add",         (void*)JSONItem_Add,        2, {"integer","string"},           "void"      },
    { "AddAt",       (void*)JSONItem_AddAt,      3, {"integer","integer","string"}, "void"      },
    { "Value",       (void*)JSONItem_Value,      2, {"integer","string"},           "string"    },
    { "SetValue",    (void*)JSONItem_SetValue,   3, {"integer","string","string"},  "void"      },
    { "ValueAt",     (void*)JSONItem_ValueAt,    2, {"integer","integer"},          "string"    },
    { "RemoveAt",    (void*)JSONItem_RemoveAt,   2, {"integer","integer"},          "void"      },
    { "Child",       (void*)JSONItem_Child,      2, {"integer","string"},           "JSONItem"  },
    { "SetChild",    (void*)JSONItem_SetChild,   3, {"integer","string","JSONItem"},"void"      },
    { "Close",       (void*)JSONItem_Destroy,    1, {"integer"},                    "void"      }
};

static ClassDefinition classDef = {
    "JSONItem", sizeof(JSONItem),
    (void*)NewJSONItem,
    props,   sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    nullptr, 0
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &classDef;
}

} // extern "C"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
#endif