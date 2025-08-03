/*

  TextInputStream.cpp
  CrossBasic Plugin: TextInputStream                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
// TextInputStream Class Declaration
// Each instance encapsulates an input file stream (std::ifstream) along with a file path.
//------------------------------------------------------------------------------
class TextInputStream {
public:
    std::string filePath; // File to be read

private:
    std::ifstream* file;
    bool isOpen;

public:
    TextInputStream() : filePath(""), file(nullptr), isOpen(false) {}

    // Opens the file using the stored filePath.
    bool Open() {
        if (isOpen) return true;
        file = new std::ifstream(filePath.c_str());
        if (file->is_open()) {
            isOpen = true;
            return true;
        }
        delete file;
        file = nullptr;
        return false;
    }

    // Reads a single line from the file.
    // Returns a newly allocated C string (caller is responsible for freeing it).
    const char* ReadLine() {
        static thread_local std::string line;
        if (!isOpen || file == nullptr || !file->is_open())
            return "";
        if (!std::getline(*file, line))
            return "";
        return strdup(line.c_str());
    }

    // Reads the entire remaining file content.
    // Returns a newly allocated C string (caller is responsible for freeing it).
    const char* ReadAll() {
        static thread_local std::string content;
        if (!isOpen || file == nullptr || !file->is_open())
            return "";
        content.assign((std::istreambuf_iterator<char>(*file)), std::istreambuf_iterator<char>());
        return strdup(content.c_str());
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

    ~TextInputStream() {
        Close();
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for TextInputStream Objects
//------------------------------------------------------------------------------
static std::mutex textInputStreamMutex;
static std::unordered_map<int, TextInputStream*> textInputStreamMap;
static std::atomic<int> currentTextInputStreamHandle(1); // Unique handle generator

//------------------------------------------------------------------------------
// Constructor: Creates a new TextInputStream instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    int handle = currentTextInputStreamHandle.fetch_add(1);
    TextInputStream* newStream = new TextInputStream();
    {
        std::lock_guard<std::mutex> lock(textInputStreamMutex);
        textInputStreamMap[handle] = newStream;
    }
    return handle;
}

//------------------------------------------------------------------------------
// Property Accessors for "FilePath"
//------------------------------------------------------------------------------

// Getter: Returns the stored file path (as a newly allocated C string).
extern "C" XPLUGIN_API const char* TextInputStream_GetFilePath(int handle) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        return strdup(it->second->filePath.c_str());
    }
    return strdup("");
}

// Setter: Updates the stored file path.
extern "C" XPLUGIN_API void TextInputStream_SetFilePath(int handle, const char* newPath) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        it->second->filePath = newPath;
    }
}

//------------------------------------------------------------------------------
// Instance Methods
// Each exported function expects the first parameter to be the instance handle.
//------------------------------------------------------------------------------

// Opens the file stream.
extern "C" XPLUGIN_API bool TextInputStream_Open(int handle) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        return it->second->Open();
    }
    return false;
}

// Reads a single line from the file.
extern "C" XPLUGIN_API const char* TextInputStream_ReadLine(int handle) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        return it->second->ReadLine();
    }
    return "";
}

// Reads the entire file content.
extern "C" XPLUGIN_API const char* TextInputStream_ReadAll(int handle) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        return it->second->ReadAll();
    }
    return "";
}

// Checks if the file stream has reached EOF.
extern "C" XPLUGIN_API int TextInputStream_EOF(int handle) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        return it->second->AtEOF();
    }
    return -1;
}

// Closes the file stream and destroys the instance.
extern "C" XPLUGIN_API void TextInputStream_Close(int handle) {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    auto it = textInputStreamMap.find(handle);
    if (it != textInputStreamMap.end()) {
        it->second->Close();
        delete it->second;
        textInputStreamMap.erase(it);
    }
}

//------------------------------------------------------------------------------
// Cleanup function to destroy all TextInputStream instances when the library unloads.
//------------------------------------------------------------------------------
void CleanupTextInputStreams() {
    std::lock_guard<std::mutex> lock(textInputStreamMutex);
    for (auto& pair : textInputStreamMap) {
        if (pair.second) {
            pair.second->Close();
            delete pair.second;
        }
    }
    textInputStreamMap.clear();
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
// Define the class properties for TextInputStream.
//------------------------------------------------------------------------------
static ClassProperty TextInputStreamProperties[] = {
    { "FilePath", "string", (void*)TextInputStream_GetFilePath, (void*)TextInputStream_SetFilePath }
};

//------------------------------------------------------------------------------
// Define the class methods for TextInputStream.
//------------------------------------------------------------------------------
static ClassEntry TextInputStreamMethods[] = {
    { "Open", (void*)TextInputStream_Open, 1, {"integer"}, "boolean" },
    { "ReadLine", (void*)TextInputStream_ReadLine, 1, {"integer"}, "string" },
    { "ReadAll", (void*)TextInputStream_ReadAll, 1, {"integer"}, "string" },
    { "EOF", (void*)TextInputStream_EOF, 1, {"integer"}, "integer" },
    { "Close", (void*)TextInputStream_Close, 1, {"integer"}, "void" }
};

// No class constants defined.
static ClassConstant TextInputStreamConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for TextInputStream.
//------------------------------------------------------------------------------
static ClassDefinition TextInputStreamClass = {
    "TextInputStream",                                 // className
    sizeof(TextInputStream),                           // classSize
    (void*)Constructor,                                // constructor (named "Constructor")
    TextInputStreamProperties,                         // properties
    sizeof(TextInputStreamProperties) / sizeof(ClassProperty), // propertiesCount
    TextInputStreamMethods,                            // methods
    sizeof(TextInputStreamMethods) / sizeof(ClassEntry),       // methodsCount
    TextInputStreamConstants,                          // constants
    sizeof(TextInputStreamConstants) / sizeof(ClassConstant)   // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &TextInputStreamClass;
}

//------------------------------------------------------------------------------
// DLL Main / Destructor to clean up instances when the library unloads.
//------------------------------------------------------------------------------
#ifdef _WIN32
extern "C" BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        CleanupTextInputStreams();
    }
    return TRUE;
}
#else
__attribute__((destructor))
static void on_unload() {
    CleanupTextInputStreams();
}
#endif
