// BinaryOutputStream.cpp
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
// BinaryOutputStream Class Declaration
// Each instance encapsulates an output binary file stream (std::ofstream)
//------------------------------------------------------------------------------
class BinaryOutputStream {
public:
    std::string filePath;  // File path for output
    bool append;           // Append mode flag

private:
    std::ofstream* file;
    bool isOpen;

public:
    BinaryOutputStream() : filePath(""), append(false), file(nullptr), isOpen(false) {}

    // Opens the binary file for writing.
    // Uses std::ios::app if append is true; otherwise std::ios::trunc.
    bool Open() {
        if (isOpen) return true;
        std::ios_base::openmode mode = std::ios::binary;
        mode |= (append ? std::ios::app : std::ios::trunc);
        file = new std::ofstream(filePath.c_str(), mode);
        if (file->is_open()) {
            isOpen = true;
            return true;
        }
        delete file;
        file = nullptr;
        return false;
    }

    // Writes a single byte (0-255)
    void WriteByte(int value) {
        if (isOpen && file && file->is_open()) {
            char byte = static_cast<char>(value);
            file->write(&byte, sizeof(byte));
        }
    }

    // Writes a 2-byte short integer
    void WriteShort(int value) {
        if (isOpen && file && file->is_open()) {
            short shortValue = static_cast<short>(value);
            file->write(reinterpret_cast<char*>(&shortValue), sizeof(shortValue));
        }
    }

    // Writes a 4-byte long integer
    void WriteLong(int value) {
        if (isOpen && file && file->is_open()) {
            file->write(reinterpret_cast<char*>(&value), sizeof(value));
        }
    }

    // Writes an 8-byte double
    void WriteDouble(double value) {
        if (isOpen && file && file->is_open()) {
            file->write(reinterpret_cast<char*>(&value), sizeof(value));
        }
    }

    // Writes a string as binary data (without writing its length)
    void WriteString(const char* text) {
        if (isOpen && file && file->is_open()) {
            size_t length = std::strlen(text);
            file->write(text, length);
        }
    }

    // Returns the current position in the file
    int Position() {
        if (!isOpen || !file || !file->is_open()) {
            return -1;
        }
        return static_cast<int>(file->tellp());
    }

    // Seeks to a specific position in the file
    void Seek(int position) {
        if (isOpen && file && file->is_open() && position >= 0) {
            file->seekp(position, std::ios::beg);
        }
    }

    // Flushes the output buffer
    void Flush() {
        if (isOpen && file && file->is_open()) {
            file->flush();
        }
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

    ~BinaryOutputStream() {
        Close();
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for BinaryOutputStream Objects
//------------------------------------------------------------------------------
static std::mutex binaryOutputStreamMutex;
static std::unordered_map<int, BinaryOutputStream*> binaryOutputStreamMap;
static std::atomic<int> currentBinaryOutputStreamHandle(1); // Unique handle generator

//------------------------------------------------------------------------------
// Constructor: Creates a new BinaryOutputStream instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    int handle = currentBinaryOutputStreamHandle.fetch_add(1);
    BinaryOutputStream* newStream = new BinaryOutputStream();
    {
        std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
        binaryOutputStreamMap[handle] = newStream;
    }
    return handle;
}

//------------------------------------------------------------------------------
// Property Accessors for "FilePath" and "Append"
//------------------------------------------------------------------------------

// Getter for "FilePath"
extern "C" XPLUGIN_API const char* BinaryOutputStream_GetFilePath(int handle) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        return strdup(it->second->filePath.c_str());
    }
    return strdup("");
}

// Setter for "FilePath"
extern "C" XPLUGIN_API void BinaryOutputStream_SetFilePath(int handle, const char* newPath) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->filePath = newPath;
    }
}

// Getter for "Append"
extern "C" XPLUGIN_API bool BinaryOutputStream_GetAppend(int handle) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        return it->second->append;
    }
    return false;
}

// Setter for "Append"
extern "C" XPLUGIN_API void BinaryOutputStream_SetAppend(int handle, bool append) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->append = append;
    }
}

//------------------------------------------------------------------------------
// Instance Methods
//------------------------------------------------------------------------------

// Opens the binary file stream.
extern "C" XPLUGIN_API bool BinaryOutputStream_Open(int handle) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        return it->second->Open();
    }
    return false;
}

// Writes a single byte.
extern "C" XPLUGIN_API void BinaryOutputStream_WriteByte(int handle, int value) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->WriteByte(value);
    }
}

// Writes a short integer.
extern "C" XPLUGIN_API void BinaryOutputStream_WriteShort(int handle, int value) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->WriteShort(value);
    }
}

// Writes a long integer.
extern "C" XPLUGIN_API void BinaryOutputStream_WriteLong(int handle, int value) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->WriteLong(value);
    }
}

// Writes a double.
extern "C" XPLUGIN_API void BinaryOutputStream_WriteDouble(int handle, double value) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->WriteDouble(value);
    }
}

// Writes a string.
extern "C" XPLUGIN_API void BinaryOutputStream_WriteString(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->WriteString(text);
    }
}

// Returns the current position in the file.
extern "C" XPLUGIN_API int BinaryOutputStream_Position(int handle) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        return it->second->Position();
    }
    return -1;
}

// Seeks to a specified position.
extern "C" XPLUGIN_API void BinaryOutputStream_Seek(int handle, int position) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->Seek(position);
    }
}

// Flushes the output buffer.
extern "C" XPLUGIN_API void BinaryOutputStream_Flush(int handle) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->Flush();
    }
}

// Closes the binary file stream and destroys the instance.
extern "C" XPLUGIN_API void BinaryOutputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    auto it = binaryOutputStreamMap.find(handle);
    if (it != binaryOutputStreamMap.end()) {
        it->second->Close();
        delete it->second;
        binaryOutputStreamMap.erase(it);
    }
}

//------------------------------------------------------------------------------
// Cleanup function to destroy all BinaryOutputStream instances when the library unloads.
//------------------------------------------------------------------------------
void CleanupBinaryOutputStreams() {
    std::lock_guard<std::mutex> lock(binaryOutputStreamMutex);
    for (auto& pair : binaryOutputStreamMap) {
        if (pair.second) {
            pair.second->Close();
            delete pair.second;
        }
    }
    binaryOutputStreamMap.clear();
}

//------------------------------------------------------------------------------
// Class Definition Structures (mirroring the MyInstance, FolderItem, and TextOutputStream demos)
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
// Define the class properties for BinaryOutputStream.
//------------------------------------------------------------------------------
static ClassProperty BinaryOutputStreamProperties[] = {
    { "FilePath", "string", (void*)BinaryOutputStream_GetFilePath, (void*)BinaryOutputStream_SetFilePath },
    { "Append", "boolean", (void*)BinaryOutputStream_GetAppend, (void*)BinaryOutputStream_SetAppend }
};

//------------------------------------------------------------------------------
// Define the class methods for BinaryOutputStream.
//------------------------------------------------------------------------------
static ClassEntry BinaryOutputStreamMethods[] = {
    { "Open", (void*)BinaryOutputStream_Open, 1, {"integer"}, "boolean" },
    { "WriteByte", (void*)BinaryOutputStream_WriteByte, 2, {"integer", "integer"}, "void" },
    { "WriteShort", (void*)BinaryOutputStream_WriteShort, 2, {"integer", "integer"}, "void" },
    { "WriteLong", (void*)BinaryOutputStream_WriteLong, 2, {"integer", "integer"}, "void" },
    { "WriteDouble", (void*)BinaryOutputStream_WriteDouble, 2, {"integer", "double"}, "void" },
    { "WriteString", (void*)BinaryOutputStream_WriteString, 2, {"integer", "string"}, "void" },
    { "Position", (void*)BinaryOutputStream_Position, 1, {"integer"}, "integer" },
    { "Seek", (void*)BinaryOutputStream_Seek, 2, {"integer", "integer"}, "void" },
    { "Flush", (void*)BinaryOutputStream_Flush, 1, {"integer"}, "void" },
    { "Close", (void*)BinaryOutputStream_Close, 1, {"integer"}, "void" }
};

// No class constants defined.
static ClassConstant BinaryOutputStreamConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for BinaryOutputStream.
//------------------------------------------------------------------------------
static ClassDefinition BinaryOutputStreamClass = {
    "BinaryOutputStream",                                      // className
    sizeof(BinaryOutputStream),                                // classSize
    (void*)Constructor,                                        // constructor (named "Constructor")
    BinaryOutputStreamProperties,                              // properties
    sizeof(BinaryOutputStreamProperties) / sizeof(ClassProperty),// propertiesCount
    BinaryOutputStreamMethods,                                 // methods
    sizeof(BinaryOutputStreamMethods) / sizeof(ClassEntry),    // methodsCount
    BinaryOutputStreamConstants,                               // constants
    sizeof(BinaryOutputStreamConstants) / sizeof(ClassConstant) // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &BinaryOutputStreamClass;
}

//------------------------------------------------------------------------------
// DLL Main / Destructor to clean up instances when the library unloads.
//------------------------------------------------------------------------------
#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        CleanupBinaryOutputStreams();
    }
    return TRUE;
}
#else
__attribute__((destructor))
static void on_unload() {
    CleanupBinaryOutputStreams();
}
#endif
