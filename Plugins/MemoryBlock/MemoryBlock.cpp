// MemoryBlock.cpp
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

#include <iostream>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <mutex>
#include <atomic>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

//------------------------------------------------------------------------------
// MemoryBlock Class Declaration
// This class encapsulates a memory block stored in a std::vector<char>.
//------------------------------------------------------------------------------
class MemoryBlock {
public:
    std::vector<char> block;

    MemoryBlock() { }

    // Resizes the memory block to newSize.
    void Resize(int newSize) {
        if (newSize > 0)
            block.resize(newSize);
    }

    // Returns the current size of the memory block.
    int Size() {
        return static_cast<int>(block.size());
    }

    // Reads a single byte at the given offset.
    int ReadByte(int offset) {
        if (offset < 0 || offset >= Size()) return -1;
        return static_cast<unsigned char>(block[offset]);
    }

    // Reads a 2-byte short integer at the given offset.
    int ReadShort(int offset) {
        if (offset < 0 || offset + 1 >= Size()) return -1;
        short value;
        std::memcpy(&value, &block[offset], sizeof(value));
        return value;
    }

    // Reads a 4-byte long integer at the given offset.
    int ReadLong(int offset) {
        if (offset < 0 || offset + 3 >= Size()) return -1;
        int value;
        std::memcpy(&value, &block[offset], sizeof(value));
        return value;
    }

    // Reads an 8-byte double at the given offset.
    double ReadDouble(int offset) {
        if (offset < 0 || offset + 7 >= Size()) return -1.0;
        double value;
        std::memcpy(&value, &block[offset], sizeof(value));
        return value;
    }

    // Reads a string of the specified length starting at offset.
    const char* ReadString(int offset, int length) {
        static thread_local std::string buffer;
        if (offset < 0 || length < 0 || offset + length > Size()) return "";
        buffer.assign(block.begin() + offset, block.begin() + offset + length);
        return buffer.c_str();
    }

    // Writes a single byte (0-255) at the given offset.
    void WriteByte(int offset, int value) {
        if (offset < 0 || offset >= Size()) return;
        block[offset] = static_cast<char>(value);
    }

    // Writes a 2-byte short integer at the given offset.
    void WriteShort(int offset, int value) {
        if (offset < 0 || offset + 1 >= Size()) return;
        short shortVal = static_cast<short>(value);
        std::memcpy(&block[offset], &shortVal, sizeof(shortVal));
    }

    // Writes a 4-byte long integer at the given offset.
    void WriteLong(int offset, int value) {
        if (offset < 0 || offset + 3 >= Size()) return;
        std::memcpy(&block[offset], &value, sizeof(value));
    }

    // Writes an 8-byte double at the given offset.
    void WriteDouble(int offset, double value) {
        if (offset < 0 || offset + 7 >= Size()) return;
        std::memcpy(&block[offset], &value, sizeof(value));
    }

    // Writes a string at the given offset.
    void WriteString(int offset, const char* value) {
        int len = std::strlen(value);
        if (offset < 0 || offset + len > Size()) return;
        std::memcpy(&block[offset], value, len);
    }

    // Copies 'length' bytes from the source memory block (srcBlock) starting at srcOffset 
    // into this memory block at destOffset.
    void CopyData(int destOffset, MemoryBlock* srcBlock, int srcOffset, int length) {
        if (!srcBlock) return;
        if (destOffset < 0 || srcOffset < 0) return;
        if (destOffset + length > Size() || srcOffset + length > srcBlock->Size()) return;
        std::memcpy(&block[destOffset], &srcBlock->block[srcOffset], length);
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for MemoryBlock Objects
//------------------------------------------------------------------------------
static std::mutex memoryMutex;
static std::unordered_map<int, MemoryBlock*> memoryBlockMap;
static std::atomic<int> currentMemoryBlockHandle(1); // Unique handle generator

//------------------------------------------------------------------------------
// Constructor: Creates a new MemoryBlock instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    int handle = currentMemoryBlockHandle.fetch_add(1);
    MemoryBlock* newBlock = new MemoryBlock();
    {
        std::lock_guard<std::mutex> lock(memoryMutex);
        memoryBlockMap[handle] = newBlock;
    }
    return handle;
}

//------------------------------------------------------------------------------
// Instance Methods
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int MemoryBlock_Size(int handle) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    return (it != memoryBlockMap.end()) ? it->second->Size() : -1;
}

extern "C" XPLUGIN_API void MemoryBlock_Resize(int handle, int newSize) {
    if (newSize <= 0) return;
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end()) {
        it->second->Resize(newSize);
    }
}

extern "C" XPLUGIN_API void MemoryBlock_Destroy(int handle) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end()) {
        delete it->second;
        memoryBlockMap.erase(it);
    }
}

extern "C" XPLUGIN_API int MemoryBlock_ReadByte(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    return (it != memoryBlockMap.end()) ? it->second->ReadByte(offset) : -1;
}

extern "C" XPLUGIN_API int MemoryBlock_ReadShort(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    return (it != memoryBlockMap.end()) ? it->second->ReadShort(offset) : -1;
}

extern "C" XPLUGIN_API int MemoryBlock_ReadLong(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    return (it != memoryBlockMap.end()) ? it->second->ReadLong(offset) : -1;
}

extern "C" XPLUGIN_API double MemoryBlock_ReadDouble(int handle, int offset) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    return (it != memoryBlockMap.end()) ? it->second->ReadDouble(offset) : -1.0;
}

extern "C" XPLUGIN_API const char* MemoryBlock_ReadString(int handle, int offset, int length) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    return (it != memoryBlockMap.end()) ? it->second->ReadString(offset, length) : "";
}

extern "C" XPLUGIN_API void MemoryBlock_WriteByte(int handle, int offset, int value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end())
        it->second->WriteByte(offset, value);
}

extern "C" XPLUGIN_API void MemoryBlock_WriteShort(int handle, int offset, int value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end())
        it->second->WriteShort(offset, value);
}

extern "C" XPLUGIN_API void MemoryBlock_WriteLong(int handle, int offset, int value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end())
        it->second->WriteLong(offset, value);
}

extern "C" XPLUGIN_API void MemoryBlock_WriteDouble(int handle, int offset, double value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end())
        it->second->WriteDouble(offset, value);
}

extern "C" XPLUGIN_API void MemoryBlock_WriteString(int handle, int offset, const char* value) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto it = memoryBlockMap.find(handle);
    if (it != memoryBlockMap.end())
        it->second->WriteString(offset, value);
}

// Renamed from "CopyMemory" to "CopyData" to avoid conflict with Windows macros.
extern "C" XPLUGIN_API void MemoryBlock_CopyData(int destHandle, int destOffset, int srcHandle, int srcOffset, int length) {
    std::lock_guard<std::mutex> lock(memoryMutex);
    auto destIt = memoryBlockMap.find(destHandle);
    auto srcIt = memoryBlockMap.find(srcHandle);
    if (destIt != memoryBlockMap.end() && srcIt != memoryBlockMap.end()) {
        destIt->second->CopyData(destOffset, srcIt->second, srcOffset, length);
    }
}

//------------------------------------------------------------------------------
// Class Definition Structures
//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
// Define the class properties for MemoryBlock.
// Expose a read-only "Size" property.
static ClassProperty MemoryBlockProperties[] = {
    { "Size", "integer", (void*)MemoryBlock_Size, nullptr }
};

//------------------------------------------------------------------------------
// Define the class methods for MemoryBlock.
// Note: The copy function is named "CopyData" to avoid name conflicts.
static ClassEntry MemoryBlockMethods[] = {
    { "Resize", (void*)MemoryBlock_Resize, 2, {"integer", "integer"}, "void" },
    { "Close", (void*)MemoryBlock_Destroy, 1, {"integer"}, "boolean" },
    { "ReadByte", (void*)MemoryBlock_ReadByte, 2, {"integer", "integer"}, "integer" },
    { "ReadShort", (void*)MemoryBlock_ReadShort, 2, {"integer", "integer"}, "integer" },
    { "ReadLong", (void*)MemoryBlock_ReadLong, 2, {"integer", "integer"}, "integer" },
    { "ReadDouble", (void*)MemoryBlock_ReadDouble, 2, {"integer", "integer"}, "double" },
    { "ReadString", (void*)MemoryBlock_ReadString, 3, {"integer", "integer", "integer"}, "string" },
    { "WriteByte", (void*)MemoryBlock_WriteByte, 3, {"integer", "integer", "integer"}, "void" },
    { "WriteShort", (void*)MemoryBlock_WriteShort, 3, {"integer", "integer", "integer"}, "void" },
    { "WriteLong", (void*)MemoryBlock_WriteLong, 3, {"integer", "integer", "integer"}, "void" },
    { "WriteDouble", (void*)MemoryBlock_WriteDouble, 3, {"integer", "integer", "double"}, "void" },
    { "WriteString", (void*)MemoryBlock_WriteString, 3, {"integer", "integer", "string"}, "void" },
    { "CopyData", (void*)MemoryBlock_CopyData, 5, {"integer", "integer", "integer", "integer", "integer"}, "void" }
};

// No class constants defined.
static ClassConstant MemoryBlockConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for MemoryBlock.
//------------------------------------------------------------------------------
static ClassDefinition MemoryBlockClass = {
    "MemoryBlock",                                      // className
    sizeof(MemoryBlock),                                // classSize
    (void*)Constructor,                                 // constructor (named "Constructor")
    MemoryBlockProperties,                              // properties
    sizeof(MemoryBlockProperties) / sizeof(ClassProperty), // propertiesCount
    MemoryBlockMethods,                                 // methods
    sizeof(MemoryBlockMethods) / sizeof(ClassEntry),    // methodsCount
    MemoryBlockConstants,                               // constants
    sizeof(MemoryBlockConstants) / sizeof(ClassConstant) // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &MemoryBlockClass;
}

#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch(ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif
