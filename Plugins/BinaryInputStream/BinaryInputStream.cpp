// BinaryInputStream.cpp
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
#include <fstream>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <cstring>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
  #define strdup _strdup
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

//------------------------------------------------------------------------------
// BinaryInputStream Class Declaration
// Each instance encapsulates an input file stream (std::ifstream) for binary reading.
//------------------------------------------------------------------------------
class BinaryInputStream {
public:
    std::string filePath;  // The file path to be read

private:
    std::ifstream* file;
    bool isOpen;

public:
    BinaryInputStream() : filePath(""), file(nullptr), isOpen(false) {}

    // Opens the file in binary mode.
    bool Open() {
        if (isOpen)
            return true;
        file = new std::ifstream(filePath.c_str(), std::ios::binary);
        if (file->is_open()) {
            isOpen = true;
            return true;
        }
        delete file;
        file = nullptr;
        return false;
    }

    // Reads a single byte (returns 0-255; returns -1 on error).
    int ReadByte() {
        if (!isOpen || file == nullptr || !file->is_open())
            return -1;
        char byte;
        file->read(&byte, 1);
        return (file->gcount() == 1) ? static_cast<unsigned char>(byte) : -1;
    }

    // Reads a 2-byte short integer.
    int ReadShort() {
        if (!isOpen || file == nullptr || !file->is_open())
            return -1;
        short value;
        file->read(reinterpret_cast<char*>(&value), sizeof(value));
        return (file->gcount() == sizeof(value)) ? value : -1;
    }

    // Reads a 4-byte long integer.
    int ReadLong() {
        if (!isOpen || file == nullptr || !file->is_open())
            return -1;
        int value;
        file->read(reinterpret_cast<char*>(&value), sizeof(value));
        return (file->gcount() == sizeof(value)) ? value : -1;
    }

    // Reads an 8-byte double.
    double ReadDouble() {
        if (!isOpen || file == nullptr || !file->is_open())
            return -1.0;
        double value;
        file->read(reinterpret_cast<char*>(&value), sizeof(value));
        return (file->gcount() == sizeof(value)) ? value : -1.0;
    }

    // Reads a specified number of bytes as a string.
    // Returns a pointer to an internal buffer (do not free).
    const char* ReadString(int length) {
        static thread_local std::string buffer;
        if (!isOpen || file == nullptr || !file->is_open() || length <= 0)
            return "";
        buffer.resize(length);
        file->read(&buffer[0], length);
        buffer.resize(file->gcount());
        return buffer.c_str();
    }

    // Returns the current position in the file.
    int Position() {
        if (!isOpen || file == nullptr || !file->is_open())
            return -1;
        return static_cast<int>(file->tellg());
    }

    // Seeks to a specific position in the file.
    void Seek(int position) {
        if (isOpen && file != nullptr && file->is_open() && position >= 0) {
            file->seekg(position, std::ios::beg);
        }
    }

    // Returns 1 if EOF is reached, 0 otherwise. Returns -1 on error.
    int AtEOF() {
        if (!isOpen || file == nullptr || !file->is_open())
            return -1;
        return file->eof() ? 1 : 0;
    }

    // Closes the file and cleans up resources.
    void Close() {
        if (isOpen && file) {
            if (file->is_open()) {
                file->close();
            }
            delete file;
            file = nullptr;
            isOpen = false;
        }
    }

    ~BinaryInputStream() {
        Close();
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for BinaryInputStream Objects
//------------------------------------------------------------------------------
static std::mutex binaryInputStreamMutex;
static std::unordered_map<int, BinaryInputStream*> binaryInputStreamMap;
static std::atomic<int> currentBinaryInputStreamHandle(1); // Unique handle generator

//------------------------------------------------------------------------------
// Constructor: Creates a new BinaryInputStream instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    int handle = currentBinaryInputStreamHandle.fetch_add(1);
    BinaryInputStream* newStream = new BinaryInputStream();
    {
        std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
        binaryInputStreamMap[handle] = newStream;
    }
    return handle;
}

//------------------------------------------------------------------------------
// Property Accessors for "FilePath"
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API const char* BinaryInputStream_GetFilePath(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return strdup(it->second->filePath.c_str());
    }
    return strdup("");
}

extern "C" XPLUGIN_API void BinaryInputStream_SetFilePath(int handle, const char* newPath) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        it->second->filePath = newPath;
    }
}

//------------------------------------------------------------------------------
// Instance Methods
// Each exported function expects the first parameter to be the instance handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API bool BinaryInputStream_Open(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->Open();
    }
    return false;
}

extern "C" XPLUGIN_API int BinaryInputStream_ReadByte(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->ReadByte();
    }
    return -1;
}

extern "C" XPLUGIN_API int BinaryInputStream_ReadShort(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->ReadShort();
    }
    return -1;
}

extern "C" XPLUGIN_API int BinaryInputStream_ReadLong(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->ReadLong();
    }
    return -1;
}

extern "C" XPLUGIN_API double BinaryInputStream_ReadDouble(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->ReadDouble();
    }
    return -1.0;
}

extern "C" XPLUGIN_API const char* BinaryInputStream_ReadString(int handle, int length) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->ReadString(length);
    }
    return "";
}

extern "C" XPLUGIN_API int BinaryInputStream_Position(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->Position();
    }
    return -1;
}

extern "C" XPLUGIN_API void BinaryInputStream_Seek(int handle, int position) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        it->second->Seek(position);
    }
}

extern "C" XPLUGIN_API int BinaryInputStream_EOF(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        return it->second->AtEOF();
    }
    return -1;
}

extern "C" XPLUGIN_API void BinaryInputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    auto it = binaryInputStreamMap.find(handle);
    if (it != binaryInputStreamMap.end()) {
        it->second->Close();
        delete it->second;
        binaryInputStreamMap.erase(it);
    }
}

//------------------------------------------------------------------------------
// Cleanup function to destroy all BinaryInputStream instances when the library unloads.
//------------------------------------------------------------------------------
void CleanupBinaryInputStreams() {
    std::lock_guard<std::mutex> lock(binaryInputStreamMutex);
    for (auto& pair : binaryInputStreamMap) {
        if (pair.second) {
            pair.second->Close();
            delete pair.second;
        }
    }
    binaryInputStreamMap.clear();
}

//------------------------------------------------------------------------------
// Class Definition Structures (mirroring previous plugins)
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
// Define the class properties for BinaryInputStream.
//------------------------------------------------------------------------------
static ClassProperty BinaryInputStreamProperties[] = {
    { "FilePath", "string", (void*)BinaryInputStream_GetFilePath, (void*)BinaryInputStream_SetFilePath }
};

//------------------------------------------------------------------------------
// Define the class methods for BinaryInputStream.
//------------------------------------------------------------------------------
static ClassEntry BinaryInputStreamMethods[] = {
    { "Open", (void*)BinaryInputStream_Open, 1, {"integer"}, "boolean" },
    { "ReadByte", (void*)BinaryInputStream_ReadByte, 1, {"integer"}, "integer" },
    { "ReadShort", (void*)BinaryInputStream_ReadShort, 1, {"integer"}, "integer" },
    { "ReadLong", (void*)BinaryInputStream_ReadLong, 1, {"integer"}, "integer" },
    { "ReadDouble", (void*)BinaryInputStream_ReadDouble, 1, {"integer"}, "double" },
    { "ReadString", (void*)BinaryInputStream_ReadString, 2, {"integer", "integer"}, "string" },
    { "Position", (void*)BinaryInputStream_Position, 1, {"integer"}, "integer" },
    { "Seek", (void*)BinaryInputStream_Seek, 2, {"integer", "integer"}, "void" },
    { "EOF", (void*)BinaryInputStream_EOF, 1, {"integer"}, "integer" },
    { "Close", (void*)BinaryInputStream_Close, 1, {"integer"}, "void" }
};

// No class constants defined.
static ClassConstant BinaryInputStreamConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for BinaryInputStream.
//------------------------------------------------------------------------------
static ClassDefinition BinaryInputStreamClass = {
    "BinaryInputStream",                                      // className
    sizeof(BinaryInputStream),                                // classSize
    (void*)Constructor,                                       // constructor (named "Constructor")
    BinaryInputStreamProperties,                              // properties
    sizeof(BinaryInputStreamProperties) / sizeof(ClassProperty), // propertiesCount
    BinaryInputStreamMethods,                                 // methods
    sizeof(BinaryInputStreamMethods) / sizeof(ClassEntry),    // methodsCount
    BinaryInputStreamConstants,                               // constants
    sizeof(BinaryInputStreamConstants) / sizeof(ClassConstant) // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &BinaryInputStreamClass;
}

//------------------------------------------------------------------------------
// DLL Main / Destructor to clean up instances when the library unloads.
//------------------------------------------------------------------------------
#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        CleanupBinaryInputStreams();
    }
    return TRUE;
}
#else
__attribute__((destructor))
static void on_unload() {
    CleanupBinaryInputStreams();
}
#endif
