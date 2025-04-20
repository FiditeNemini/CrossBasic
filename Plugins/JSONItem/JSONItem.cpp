// JSONItem.cpp
// Created by Matthew A Combatti  
// Simulanics Technologies and Xojo Developers Studio  
// https://www.simulanics.com  
// https://www.xojostudio.org  
// DISCLAIMER: Simulanics Technologies and Xojo Developers Studio are not affiliated with Xojo, Inc.
// -----------------------------------------------------------------------------  
// Copyright (c) 2025 Simulanics Technologies and Xojo Developers Studio  
//  
// Permission is hereby granted, free of charge, to any person obtaining a copy  
// of this software and associated documentation files (the "Software"), to deal  
// in the Software without restriction, including without limitation the rights  
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell  
// copies of the Software, and to permit persons to whom the Software is  
// furnished to do so, subject to the following conditions:  
//  
// The above copyright notice and this permission notice shall be included in all  
// copies or substantial portions of the Software.  
//  
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,  
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER  
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,  
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE  
// SOFTWARE.
// ----------------------------------------------------------------------------- 

#ifdef _WIN32
  // Prevent unnecessary definitions and conflicts.
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #define WIN32_LEAN_AND_MEAN
  #define _WINSOCKAPI_
  #include <windows.h>
  // Undefine the conflicting "byte" macro from Windows headers.
  #undef byte
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>
#include <mutex>
#include <map>
#include <atomic>
#include <cstdlib>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std;

// -----------------------------------------------------------------------------
// JSONItem Class Definition
// -----------------------------------------------------------------------------
class JSONItem {
public:
    // Public member to store the instance handle.
    int Handle;
    // Internal JSON data.
    json data;
    // Formatting properties.
    bool Compact;      // When true (default), produces compact JSON.
    int IndentSpacing; // Number of spaces for indentation when Compact is false.

    // Constructor: creates an empty JSON object.
    JSONItem() : Handle(0), data(json::object()), Compact(true), IndentSpacing(3) { }

    // Loads and parses the JSON string.
    void Load(const string &jsonString) {
        try {
            data = json::parse(jsonString);
        } catch (...) {
            throw runtime_error("Failed to parse JSON string.");
        }
    }

    // Returns a JSON string representing the current object.
    string ToString() {
        if (Compact)
            return data.dump();
        else
            return data.dump(IndentSpacing);
    }

    // Returns the number of children (keys for objects or size for arrays).
    int Count() {
        if (data.is_object() || data.is_array())
            return static_cast<int>(data.size());
        return 0;
    }

    // Returns true if the JSONItem (object) contains the specified key.
    bool HasKey(const string &key) {
        if (data.is_object())
            return data.find(key) != data.end();
        return false;
    }

    // Returns true if the JSONItem is an array.
    bool IsArray() {
        return data.is_array();
    }

    // Returns the key at the specified index (for objects).
    string KeyAt(int index) {
        if (data.is_object()) {
            int i = 0;
            for (auto it = data.begin(); it != data.end(); ++it) {
                if (i == index)
                    return it.key();
                i++;
            }
        }
        return "";
    }

    // Returns the keys as a vector of strings (for objects).
    vector<string> Keys() {
        vector<string> result;
        if (data.is_object()) {
            for (auto& el : data.items())
                result.push_back(el.key());
        }
        return result;
    }

    // Returns the last row index (for arrays).
    int LastRowIndex() {
        if (data.is_array())
            return static_cast<int>(data.size()) - 1;
        return -1;
    }

    // Looks up the value for a given key; returns defaultValue if key not found.
    string Lookup(const string &key, const string &defaultValue) {
        if (data.is_object() && data.contains(key))
            return data[key].dump();
        return defaultValue;
    }

    // Removes the child with the specified key (for objects).
    void Remove(const string &key) {
        if (data.is_object())
            data.erase(key);
    }

    // Removes all children.
    void RemoveAll() {
        data.clear();
    }

    // Adds a value to the end of the array.
    void Add(const string &value) {
        if (!data.is_array())
            data = json::array();
        data.push_back(value);
    }

    // Adds a value at the specified index in the array.
    void AddAt(int index, const string &value) {
        if (!data.is_array())
            data = json::array();
        if (index >= 0 && index <= data.size())
            data.insert(data.begin() + index, value);
    }

    // Gets the value for the given key (for objects) as a JSON string.
    string Value(const string &key) {
        if (data.is_object() && data.contains(key))
            return data[key].dump();
        return "";
    }

    // Sets the value for the given key (for objects).
    void SetValue(const string &key, const string &value) {
        if (!data.is_object())
            data = json::object();
        data[key] = value;
    }
    
    // --- New Method for Array Access ---
    // Returns the array element at the specified index as a JSON string.
    string ValueAt(int index) {
        if (data.is_array() && index >= 0 && index < data.size())
            return data[index].dump();
        throw out_of_range("Index out of range in ValueAt");
    }
    
    // Removes the array element at the specified index.
    void RemoveAt(int index) {
        if (data.is_array() && index >= 0 && index < data.size())
            data.erase(data.begin() + index);
    }
    
    // Sets a child JSONItem for the given key (stores the child’s internal JSON directly).
    void SetChild(const string &key, const JSONItem &child) {
        if (!data.is_object())
            data = json::object();
        data[key] = child.data;
    }
    
    // Gets a child JSONItem for the given key.
    JSONItem Child(const string &key) {
        JSONItem child;
        if (data.is_object() && data.contains(key))
            child.data = data[key];
        return child;
    }
};

// -----------------------------------------------------------------------------
// Global Instance Management for JSONItem Instances
// -----------------------------------------------------------------------------
static mutex jsonMutex;
static map<int, JSONItem*> jsonMap;
static atomic<int> nextJsonHandle(1);

// -----------------------------------------------------------------------------
// Exported Functions (extern "C")
// -----------------------------------------------------------------------------
extern "C" {

// Creates a new JSONItem instance and returns its unique handle.
XPLUGIN_API int NewJSONItem() {
    JSONItem* item = new JSONItem();
    int handle = nextJsonHandle.fetch_add(1);
    item->Handle = handle; // Store the handle in the instance.
    lock_guard<mutex> lock(jsonMutex);
    jsonMap[handle] = item;
    return handle;
}

// Returns the Handle property of the JSONItem instance.
XPLUGIN_API int JSONItem_GetHandle(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        return it->second->Handle;
    return 0;
}

// Sets a child JSONItem for a given key using a child's handle.
XPLUGIN_API void JSONItem_SetChild(int handle, const char* key, int childHandle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    auto childIt = jsonMap.find(childHandle);
    if (it != jsonMap.end() && childIt != jsonMap.end()) {
        it->second->SetChild(string(key), *(childIt->second));
    }
}

// Loads a JSON string into the JSONItem instance.
XPLUGIN_API const char* JSONItem_Load(int handle, const char* jsonString) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return "Error: Invalid instance handle";
    try {
        it->second->Load(string(jsonString));
        result = "OK";
    } catch (const exception &e) {
        result = e.what();
    }
    return result.c_str();
}

// Returns a JSON string representing the current JSONItem.
XPLUGIN_API const char* JSONItem_ToString(int handle) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return "";
    result = it->second->ToString();
    return result.c_str();
}

// Returns the number of children.
XPLUGIN_API int JSONItem_Count(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return 0;
    return it->second->Count();
}

// Returns true if the JSONItem has the specified key.
XPLUGIN_API bool JSONItem_HasKey(int handle, const char* key) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return false;
    return it->second->HasKey(string(key));
}

// Returns true if the JSONItem is an array.
XPLUGIN_API bool JSONItem_IsArray(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return false;
    return it->second->IsArray();
}

// Returns the keys as a JSON array string.
XPLUGIN_API const char* JSONItem_Keys(int handle) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return "[]";
    vector<string> keys = it->second->Keys();
    json j_keys = keys;
    result = j_keys.dump();
    return result.c_str();
}

// Returns the last row index (for arrays).
XPLUGIN_API int JSONItem_LastRowIndex(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return -1;
    return it->second->LastRowIndex();
}

// Looks up the value for the given key; returns defaultValue if not found.
XPLUGIN_API const char* JSONItem_Lookup(int handle, const char* key, const char* defaultValue) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return defaultValue;
    result = it->second->Lookup(string(key), string(defaultValue));
    return result.c_str();
}

// Removes the child with the specified key.
XPLUGIN_API void JSONItem_Remove(int handle, const char* key) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->Remove(string(key));
}

// Removes all children.
XPLUGIN_API void JSONItem_RemoveAll(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->RemoveAll();
}

// Adds a value to the end of the array.
XPLUGIN_API void JSONItem_Add(int handle, const char* value) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->Add(string(value));
}

// Adds a value at the specified index.
XPLUGIN_API void JSONItem_AddAt(int handle, int index, const char* value) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->AddAt(index, string(value));
}

// Gets the value for the given key.
XPLUGIN_API const char* JSONItem_Value(int handle, const char* key) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return "";
    result = it->second->Value(string(key));
    return result.c_str();
}

// Sets the value for the given key.
XPLUGIN_API void JSONItem_SetValue(int handle, const char* key, const char* value) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->SetValue(string(key), string(value));
}

// --- New Exported Method: ValueAt ---
// Returns the array element at the given index as a JSON string.
XPLUGIN_API const char* JSONItem_ValueAt(int handle, int index) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return "";
    try {
        result = it->second->ValueAt(index);
    } catch (const exception &e) {
        result = e.what();
    }
    return result.c_str();
}

// --- New Exported Method: RemoveAt ---
// Removes the array element at the specified index.
XPLUGIN_API void JSONItem_RemoveAt(int handle, int index) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->RemoveAt(index);
}

// Getter for Compact property.
XPLUGIN_API const char* JSONItem_GetCompact(int handle) {
    static string result;
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return "false";
    result = it->second->Compact ? "true" : "false";
    return result.c_str();
}

// Setter for Compact property.
XPLUGIN_API void JSONItem_SetCompact(int handle, bool value) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->Compact = value;
}

// Getter for IndentSpacing property.
XPLUGIN_API int JSONItem_GetIndentSpacing(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return 0;
    return it->second->IndentSpacing;
}

// Setter for IndentSpacing property.
XPLUGIN_API void JSONItem_SetIndentSpacing(int handle, int value) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it != jsonMap.end())
        it->second->IndentSpacing = value;
}

// Destroys the JSONItem instance.
XPLUGIN_API bool JSONItem_Destroy(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    if (it == jsonMap.end())
        return false;
    delete it->second;
    jsonMap.erase(it);
    return true;
}

// -----------------------------------------------------------------------------
// Class Definition Structures (for interpreter integration)
// -----------------------------------------------------------------------------
typedef struct {
    const char* name;
    const char* type;
    void* getter;
    void* setter;
} ClassProperty;

typedef struct {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} ClassEntry;

typedef struct {
    const char* declaration;
} ClassConstant;

typedef struct {
    const char* className;
    size_t classSize;
    void* constructor;
    ClassProperty* properties;
    size_t propertiesCount;
    ClassEntry* methods;
    size_t methodsCount;
    ClassConstant* constants;
    size_t constantsCount;
} ClassDefinition;

// Standalone getter for the ToString property.
XPLUGIN_API const char* JSONItem_ToStringGetter(int handle) {
    lock_guard<mutex> lock(jsonMutex);
    auto it = jsonMap.find(handle);
    static string s;
    if (it != jsonMap.end()) {
        s = it->second->ToString();
        return s.c_str();
    }
    return "";
}

static ClassProperty JSONItemProperties[] = {
    { "Compact", "boolean", (void*)JSONItem_GetCompact, (void*)JSONItem_SetCompact },
    { "IndentSpacing", "integer", (void*)JSONItem_GetIndentSpacing, (void*)JSONItem_SetIndentSpacing },
    { "ToString", "string", (void*)JSONItem_ToStringGetter, nullptr },
    { "Handle", "integer", (void*)JSONItem_GetHandle, nullptr }
};

static ClassEntry JSONItemMethods[] = {
    { "Load", (void*)JSONItem_Load, 2, {"integer", "string"}, "string" },
    { "Count", (void*)JSONItem_Count, 1, {"integer"}, "integer" },
    { "HasKey", (void*)JSONItem_HasKey, 2, {"integer", "string"}, "boolean" },
    { "IsArray", (void*)JSONItem_IsArray, 1, {"integer"}, "boolean" },
    { "Keys", (void*)JSONItem_Keys, 1, {"integer"}, "string" },
    { "LastRowIndex", (void*)JSONItem_LastRowIndex, 1, {"integer"}, "integer" },
    { "Lookup", (void*)JSONItem_Lookup, 3, {"integer", "string", "string"}, "string" },
    { "Remove", (void*)JSONItem_Remove, 2, {"integer", "string"}, "void" },
    { "RemoveAll", (void*)JSONItem_RemoveAll, 1, {"integer"}, "void" },
    { "Add", (void*)JSONItem_Add, 2, {"integer", "string"}, "void" },
    { "AddAt", (void*)JSONItem_AddAt, 3, {"integer", "integer", "string"}, "void" },
    { "Value", (void*)JSONItem_Value, 2, {"integer", "string"}, "string" },
    { "SetValue", (void*)JSONItem_SetValue, 3, {"integer", "string", "string"}, "void" },
    { "ValueAt", (void*)JSONItem_ValueAt, 2, {"integer", "integer"}, "string" },
    { "RemoveAt", (void*)JSONItem_RemoveAt, 2, {"integer", "integer"}, "void" },
    { "Close", (void*)JSONItem_Destroy, 1, {"integer"}, "void" },
    { "SetChild", (void*)JSONItem_SetChild, 3, {"integer", "string", "integer"}, "void" }
};

static ClassConstant JSONItemConstants[] = { };

static ClassDefinition JSONItemClass = {
    "JSONItem",
    sizeof(JSONItem),
    (void*)NewJSONItem, // Parameterless constructor.
    JSONItemProperties,
    sizeof(JSONItemProperties) / sizeof(ClassProperty),
    JSONItemMethods,
    sizeof(JSONItemMethods) / sizeof(ClassEntry),
    JSONItemConstants,
    sizeof(JSONItemConstants) / sizeof(ClassConstant)
};

XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &JSONItemClass;
}

} // extern "C"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
#endif
