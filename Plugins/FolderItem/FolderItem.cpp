// FolderItem.cpp
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
#include <string>
#include <sys/stat.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
  #include <windows.h>
  #include <direct.h>
  #define XPLUGIN_API __declspec(dllexport)
  #define PATH_SEPARATOR "\\"
  #define strdup _strdup
  #include <limits.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <limits.h>
  #define XPLUGIN_API __attribute__((visibility("default")))
  #define PATH_SEPARATOR "/"
#endif

#include <mutex>
#include <unordered_map>
#include <atomic>

//------------------------------------------------------------------------------
// FolderItem Class Declaration
// Each instance represents a file/folder identified by its stored path.
//------------------------------------------------------------------------------
class FolderItem {
public:
    std::string path; // The file or directory path

    FolderItem() : path("") {}

    // Checks if the file or directory exists.
    bool Exists() {
        struct stat info;
        return (stat(path.c_str(), &info) == 0);
    }
    
    // Deletes the file or directory.
    bool Delete() {
        struct stat info;
        if (stat(path.c_str(), &info) != 0)
            return false;
        if (S_ISDIR(info.st_mode)) {
#ifdef _WIN32
            return (_rmdir(path.c_str()) == 0);
#else
            return (rmdir(path.c_str()) == 0);
#endif
        } else {
            return (remove(path.c_str()) == 0);
        }
    }
    
    // Creates a directory at the stored path.
    bool CreateDirectory() {
#ifdef _WIN32
        return (_mkdir(path.c_str()) == 0);
#else
        return (mkdir(path.c_str(), 0777) == 0);
#endif
    }
    
    // Determines if the stored path is a directory.
    bool IsDirectory() {
        struct stat info;
        if (stat(path.c_str(), &info) != 0)
            return false;
        return S_ISDIR(info.st_mode);
    }
    
    // Returns the file size in bytes (0 for directories or on error).
    int Size() {
        struct stat info;
        if (stat(path.c_str(), &info) != 0 || S_ISDIR(info.st_mode))
            return 0;
        return static_cast<int>(info.st_size);
    }
    
    // Returns the absolute path as a newly allocated C string.
    const char* GetPath() {
        std::string fullPath;
#ifdef _WIN32
        char absolutePath[MAX_PATH];
        if (_fullpath(absolutePath, path.c_str(), MAX_PATH))
            fullPath = absolutePath;
        else
            fullPath = path;
#else
        char absolutePath[PATH_MAX];
        if (realpath(path.c_str(), absolutePath))
            fullPath = absolutePath;
        else
            fullPath = path;
#endif
        return strdup(fullPath.c_str());
    }
    
    // Returns the Unix-style file permission bits.
    int GetPermission() {
        struct stat info;
        if (stat(path.c_str(), &info) != 0)
            return -1;
        return (info.st_mode & 0777);
    }
    
    // Sets the Unix-style file permission bits.
    bool SetPermission(int permission) {
#ifdef _WIN32
        return false; // Not supported on Windows
#else
        return (chmod(path.c_str(), permission) == 0);
#endif
    }
    
    // Converts the stored path to a URL-style path (e.g., "file://...").
    const char* URLPath() {
        std::string urlPath = "file://";
#ifdef _WIN32
        urlPath += "/";
#endif
        urlPath += path;
        for (size_t i = 0; i < urlPath.size(); ++i) {
            if (urlPath[i] == '\\')
                urlPath[i] = '/';
        }
        return strdup(urlPath.c_str());
    }
    
    // Returns a shell-safe version of the stored path (escapes spaces).
    const char* ShellPath() {
        std::string result;
        for (char c : path) {
            if (c == ' ')
                result += "\\ ";
            else
                result += c;
        }
        return strdup(result.c_str());
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for FolderItem Objects
//------------------------------------------------------------------------------
static std::mutex folderItemMapMutex;
static std::unordered_map<int, FolderItem*> folderItemMap;
static std::atomic<int> currentFolderItemHandle(1); // Unique handle generator

//------------------------------------------------------------------------------
// Constructor: Creates a new FolderItem instance and returns its unique handle.
// This mirrors the MyInstance Constructor implementation.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    int handle = currentFolderItemHandle.fetch_add(1);
    FolderItem* newItem = new FolderItem();
    {
        std::lock_guard<std::mutex> lock(folderItemMapMutex);
        folderItemMap[handle] = newItem;
    }
    return handle;
}

//------------------------------------------------------------------------------
// Property Accessors for "Path"
//------------------------------------------------------------------------------

// Getter for the "Path" property: returns the stored path as a newly allocated C string.
extern "C" XPLUGIN_API const char* FolderItem_GetPathProp(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return strdup(it->second->path.c_str());
    }
    return strdup("");
}

// Setter for the "Path" property.
extern "C" XPLUGIN_API void FolderItem_SetPathProp(int handle, const char* newPath) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        it->second->path = newPath;
    }
}

//------------------------------------------------------------------------------
// Instance Methods
// Each exported function expects the first parameter to be the instance handle.
//------------------------------------------------------------------------------

extern "C" XPLUGIN_API bool FolderItem_Exists(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->Exists();
    }
    return false;
}

extern "C" XPLUGIN_API bool FolderItem_Delete(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->Delete();
    }
    return false;
}

extern "C" XPLUGIN_API bool FolderItem_CreateDirectory(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->CreateDirectory();
    }
    return false;
}

extern "C" XPLUGIN_API bool FolderItem_IsDirectory(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->IsDirectory();
    }
    return false;
}

extern "C" XPLUGIN_API int FolderItem_Size(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->Size();
    }
    return 0;
}

extern "C" XPLUGIN_API const char* FolderItem_GetPathMethod(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->GetPath();
    }
    return strdup("");
}

extern "C" XPLUGIN_API int FolderItem_GetPermission(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->GetPermission();
    }
    return -1;
}

extern "C" XPLUGIN_API bool FolderItem_SetPermission(int handle, int permission) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->SetPermission(permission);
    }
    return false;
}

extern "C" XPLUGIN_API const char* FolderItem_URLPath(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->URLPath();
    }
    return strdup("");
}

extern "C" XPLUGIN_API const char* FolderItem_ShellPath(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        return it->second->ShellPath();
    }
    return strdup("");
}

//------------------------------------------------------------------------------
// Close: Destroys a FolderItem instance and removes it from the global map.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API void FolderItem_Close(int handle) {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    auto it = folderItemMap.find(handle);
    if (it != folderItemMap.end()) {
        delete it->second;
        folderItemMap.erase(it);
    }
}

//------------------------------------------------------------------------------
// Cleanup function to destroy all FolderItem instances when the library unloads.
//------------------------------------------------------------------------------
void CleanupFolderItems() {
    std::lock_guard<std::mutex> lock(folderItemMapMutex);
    for (auto& pair : folderItemMap) {
        delete pair.second;
    }
    folderItemMap.clear();
}

//------------------------------------------------------------------------------
// Class Definition Structures (mirroring the MyInstance demo)
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
// Define the class properties for FolderItem.
//------------------------------------------------------------------------------
static ClassProperty FolderItemProperties[] = {
    { "Path", "string", (void*)FolderItem_GetPathProp, (void*)FolderItem_SetPathProp }
};

//------------------------------------------------------------------------------
// Define the class methods for FolderItem.
//------------------------------------------------------------------------------
static ClassEntry FolderItemMethods[] = {
    { "Exists", (void*)FolderItem_Exists, 1, {"integer"}, "boolean" },
    { "Delete", (void*)FolderItem_Delete, 1, {"integer"}, "boolean" },
    { "CreateDirectory", (void*)FolderItem_CreateDirectory, 1, {"integer"}, "boolean" },
    { "IsDirectory", (void*)FolderItem_IsDirectory, 1, {"integer"}, "boolean" },
    { "Size", (void*)FolderItem_Size, 1, {"integer"}, "integer" },
    { "GetPath", (void*)FolderItem_GetPathMethod, 1, {"integer"}, "string" },
    { "GetPermission", (void*)FolderItem_GetPermission, 1, {"integer"}, "integer" },
    { "SetPermission", (void*)FolderItem_SetPermission, 2, {"integer", "integer"}, "boolean" },
    { "URLPath", (void*)FolderItem_URLPath, 1, {"integer"}, "string" },
    { "ShellPath", (void*)FolderItem_ShellPath, 1, {"integer"}, "string" },
    { "Close", (void*)FolderItem_Close, 1, {"integer"}, "void" }
};

//------------------------------------------------------------------------------
// No constants are defined for FolderItem in this example.
//------------------------------------------------------------------------------
static ClassConstant FolderItemConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for FolderItem.
//------------------------------------------------------------------------------
static ClassDefinition FolderItemClass = {
    "FolderItem",                                      // className
    sizeof(FolderItem),                                // classSize
    (void*)Constructor,                                // constructor (named "Constructor" like in MyInstance)
    FolderItemProperties,                              // properties
    sizeof(FolderItemProperties) / sizeof(ClassProperty),// propertiesCount
    FolderItemMethods,                                 // methods
    sizeof(FolderItemMethods) / sizeof(ClassEntry),    // methodsCount
    FolderItemConstants,                               // constants
    sizeof(FolderItemConstants) / sizeof(ClassConstant) // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &FolderItemClass;
}

//------------------------------------------------------------------------------
// DLL Main / Destructor to clean up FolderItem instances when the library unloads.
//------------------------------------------------------------------------------
#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        CleanupFolderItems();
    }
    return TRUE;
}
#else
__attribute__((destructor))
static void on_unload() {
    CleanupFolderItems();
}
#endif
