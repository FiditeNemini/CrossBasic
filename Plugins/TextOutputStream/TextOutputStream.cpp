// TextOutputStream.cpp
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
// TextOutputStream Class Declaration
// Each instance manages an output file stream with configurable file path and append mode.
//------------------------------------------------------------------------------
class TextOutputStream {
public:
    std::string filePath; // The file path to open
    bool append;          // Determines if the file is opened in append mode

private:
    std::ofstream* file;
    bool isOpen;

public:
    TextOutputStream() : filePath(""), append(false), file(nullptr), isOpen(false) {}

    // Opens the file based on the current filePath and append properties.
    // Returns true on success; false otherwise.
    bool Open() {
        if (isOpen) return true;
        file = new std::ofstream(filePath.c_str(), append ? std::ios::app : std::ios::trunc);
        if (file->is_open()) {
            isOpen = true;
            return true;
        }
        delete file;
        file = nullptr;
        return false;
    }

    // Writes text to the file without a newline.
    void Write(const char* text) {
        if (isOpen && file && file->is_open()) {
            (*file) << text;
        }
    }

    // Writes a line to the file with a newline.
    void WriteLine(const char* text) {
        if (isOpen && file && file->is_open()) {
            (*file) << text << std::endl;
        }
    }

    // Flushes the file output.
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

    ~TextOutputStream() {
        Close();
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for TextOutputStream Objects
//------------------------------------------------------------------------------
static std::mutex textOutputStreamMutex;
static std::unordered_map<int, TextOutputStream*> textOutputStreamMap;
static std::atomic<int> currentTextOutputStreamHandle(1); // Unique handle generator

//------------------------------------------------------------------------------
// Constructor: Creates a new TextOutputStream instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    int handle = currentTextOutputStreamHandle.fetch_add(1);
    TextOutputStream* newStream = new TextOutputStream();
    {
        std::lock_guard<std::mutex> lock(textOutputStreamMutex);
        textOutputStreamMap[handle] = newStream;
    }
    return handle;
}

//------------------------------------------------------------------------------
// Property Accessors for "FilePath" and "Append"
//------------------------------------------------------------------------------

// Getter for the "FilePath" property.
extern "C" XPLUGIN_API const char* TextOutputStream_GetFilePath(int handle) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        return strdup(it->second->filePath.c_str());
    }
    return strdup("");
}

// Setter for the "FilePath" property.
extern "C" XPLUGIN_API void TextOutputStream_SetFilePath(int handle, const char* newPath) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        it->second->filePath = newPath;
    }
}

// Getter for the "Append" property.
extern "C" XPLUGIN_API bool TextOutputStream_GetAppend(int handle) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        return it->second->append;
    }
    return false;
}

// Setter for the "Append" property.
extern "C" XPLUGIN_API void TextOutputStream_SetAppend(int handle, bool append) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        it->second->append = append;
    }
}

//------------------------------------------------------------------------------
// Instance Methods
//------------------------------------------------------------------------------

// Opens the file stream using the current properties.
extern "C" XPLUGIN_API bool TextOutputStream_Open(int handle) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        return it->second->Open();
    }
    return false;
}

// Writes text to the file without adding a newline.
extern "C" XPLUGIN_API void TextOutputStream_Write(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        it->second->Write(text);
    }
}

// Writes a line to the file with a newline character.
extern "C" XPLUGIN_API void TextOutputStream_WriteLine(int handle, const char* text) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        it->second->WriteLine(text);
    }
}

// Flushes the file stream.
extern "C" XPLUGIN_API void TextOutputStream_Flush(int handle) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        it->second->Flush();
    }
}

// Closes the file stream and destroys the instance.
extern "C" XPLUGIN_API void TextOutputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    auto it = textOutputStreamMap.find(handle);
    if (it != textOutputStreamMap.end()) {
        it->second->Close();
        delete it->second;
        textOutputStreamMap.erase(it);
    }
}

//------------------------------------------------------------------------------
// Cleanup function to destroy all TextOutputStream instances when the library unloads.
//------------------------------------------------------------------------------
void CleanupTextOutputStreams() {
    std::lock_guard<std::mutex> lock(textOutputStreamMutex);
    for (auto& pair : textOutputStreamMap) {
        if (pair.second) {
            pair.second->Close();
            delete pair.second;
        }
    }
    textOutputStreamMap.clear();
}

//------------------------------------------------------------------------------
// Class Definition Structures (mirroring the MyInstance and FolderItem demos)
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
// Define the class properties for TextOutputStream.
//------------------------------------------------------------------------------
static ClassProperty TextOutputStreamProperties[] = {
    { "FilePath", "string", (void*)TextOutputStream_GetFilePath, (void*)TextOutputStream_SetFilePath },
    { "Append", "boolean", (void*)TextOutputStream_GetAppend, (void*)TextOutputStream_SetAppend }
};

//------------------------------------------------------------------------------
// Define the class methods for TextOutputStream.
//------------------------------------------------------------------------------
static ClassEntry TextOutputStreamMethods[] = {
    { "Open", (void*)TextOutputStream_Open, 1, {"integer"}, "boolean" },
    { "Write", (void*)TextOutputStream_Write, 2, {"integer", "string"}, "void" },
    { "WriteLine", (void*)TextOutputStream_WriteLine, 2, {"integer", "string"}, "void" },
    { "Flush", (void*)TextOutputStream_Flush, 1, {"integer"}, "void" },
    { "Close", (void*)TextOutputStream_Close, 1, {"integer"}, "void" }
};

// No class constants defined.
static ClassConstant TextOutputStreamConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for TextOutputStream.
//------------------------------------------------------------------------------
static ClassDefinition TextOutputStreamClass = {
    "TextOutputStream",                                     // className
    sizeof(TextOutputStream),                               // classSize
    (void*)Constructor,                                     // constructor (named "Constructor")
    TextOutputStreamProperties,                             // properties
    sizeof(TextOutputStreamProperties) / sizeof(ClassProperty), // propertiesCount
    TextOutputStreamMethods,                                // methods
    sizeof(TextOutputStreamMethods) / sizeof(ClassEntry),   // methodsCount
    TextOutputStreamConstants,                              // constants
    sizeof(TextOutputStreamConstants) / sizeof(ClassConstant) // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &TextOutputStreamClass;
}

//------------------------------------------------------------------------------
// DLL Main / Destructor to clean up instances when the library unloads.
//------------------------------------------------------------------------------
#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        CleanupTextOutputStreams();
    }
    return TRUE;
}
#else
__attribute__((destructor))
static void on_unload() {
    CleanupTextOutputStreams();
}
#endif
