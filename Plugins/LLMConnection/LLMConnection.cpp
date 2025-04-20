// LLMConnectionClass.cpp
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
//
// Build (Linux/macOS):
//    g++ -shared -fPIC -o LLMConnectionPlugin.so LLMConnectionClass.cpp -lcurl
// Build (Windows):
//    g++ -shared -o LLMConnectionPlugin.dll LLMConnectionClass.cpp -static -lcurl -lcurl.dll
#include <nlohmann/json.hpp>
#include "openai/openai.hpp"
#include <string>
#include <mutex>
#include <map>
#include <atomic>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#include <unistd.h>
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

using json = nlohmann::json;

//------------------------------------------------------------------------------
// LLMConnection Class Declaration
//------------------------------------------------------------------------------
class LLMConnection {
private:
    std::mutex instanceMutex;
public:
    // Configuration properties – set these after instantiation.
    std::string APIHost;       // e.g., "https://api.openai.com/v1/"
    std::string APIKey;        // required for OpenAI/commercial API
    std::string Organization;  // optional

    // Parameterless constructor with a default API host.
    LLMConnection() : APIHost("https://api.openai.com/v1/"), APIKey(""), Organization("") { }

    // Sets the API host.
    void SetAPIHost(const std::string &host) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        APIHost = host;
    }
    
    // Sets the API key and organization.
    void SetConfiguration(const std::string &key, const std::string &org) {
        std::lock_guard<std::mutex> lock(instanceMutex);
        APIKey = key;
        Organization = org;
    }
    
    // Creates a text completion request.
    std::string CreateCompletion(const std::string &model, const std::string &prompt, int max_tokens, double temperature) {
        if(model.empty() || prompt.empty() || max_tokens <= 0 || temperature < 0.0 || temperature > 1.0)
            return "Error: Invalid parameters.";
        
        {
            std::lock_guard<std::mutex> lock(instanceMutex);
            openai::instance().setBaseUrl(APIHost);
            if (!APIKey.empty()) {
                openai::start(APIKey.c_str(), Organization.empty() ? nullptr : Organization.c_str());
            }
        }
        
        try {
            json requestPayload = {
                { "model", model },
                { "prompt", prompt },
                { "max_tokens", max_tokens },
                { "temperature", temperature }
            };
            auto completion = openai::completion().create(requestPayload);
            return completion["choices"][0]["text"].get<std::string>();
        } catch (const std::exception& e) {
            return "Error: Exception during API request.";
        }
    }
    
    // Creates an image generation request.
    std::string CreateImage(const std::string &model, const std::string &prompt, int n, const std::string &size) {
        if(model.empty() || prompt.empty() || n <= 0 || size.empty())
            return "Error: Invalid parameters.";
        
        {
            std::lock_guard<std::mutex> lock(instanceMutex);
            openai::instance().setBaseUrl(APIHost);
            if (!APIKey.empty()) {
                openai::start(APIKey.c_str(), Organization.empty() ? nullptr : Organization.c_str());
            }
        }
        
        try {
            json requestPayload = {
                { "model", model },
                { "prompt", prompt },
                { "n", n },
                { "size", size }
            };
            auto image = openai::image().create(requestPayload);
            return image["data"][0]["url"].get<std::string>();
        } catch (const std::exception& e) {
            return "Error: Exception during API request.";
        }
    }
    
    // Returns a string representation of this connection instance.
    std::string ToString() {
        std::lock_guard<std::mutex> lock(instanceMutex);
        return "LLMConnection:" + APIHost;
    }
    
    // Closes the connection (for symmetry; nothing to do in this example).
    void Close() { }
};

//------------------------------------------------------------------------------
// Global Instance Management for LLMConnection Instances
//------------------------------------------------------------------------------
static std::mutex llmMutex;
static std::map<int, LLMConnection*> llmMap;
static std::atomic<int> nextLLMHandle(1);

//------------------------------------------------------------------------------
// Exported Functions (LLMConnection)
//------------------------------------------------------------------------------

// Constructor: Creates a new LLMConnection instance and returns its unique handle.
extern "C" XPLUGIN_API int NewLLMConnection() {
    LLMConnection* conn = new LLMConnection();
    int handle = nextLLMHandle.fetch_add(1);
    {
        std::lock_guard<std::mutex> lock(llmMutex);
        llmMap[handle] = conn;
    }
    return handle;
}

// Sets the API host.
extern "C" XPLUGIN_API void LLMSetAPIHost_Instance(int handle, const char* host) {
    std::lock_guard<std::mutex> lock(llmMutex);
    auto it = llmMap.find(handle);
    if (it != llmMap.end() && host)
        it->second->SetAPIHost(host);
}

// Sets the API key and organization.
extern "C" XPLUGIN_API void LLMSetConfiguration_Instance(int handle, const char* key, const char* org) {
    std::lock_guard<std::mutex> lock(llmMutex);
    auto it = llmMap.find(handle);
    if (it != llmMap.end()) {
        it->second->SetConfiguration(key ? key : "", org ? org : "");
    }
}

// Creates a text completion request.
extern "C" XPLUGIN_API const char* LLMCreateCompletion_Instance(int handle, const char* model, const char* prompt, int max_tokens, double temperature) {
    static std::string responseText;
    std::lock_guard<std::mutex> lock(llmMutex);
    auto it = llmMap.find(handle);
    if (it == llmMap.end() || !model || !prompt)
        return "Error: Invalid instance or parameters.";
    responseText = it->second->CreateCompletion(model, prompt, max_tokens, temperature);
    return responseText.c_str();
}

// Creates an image generation request.
extern "C" XPLUGIN_API const char* LLMCreateImage_Instance(int handle, const char* model, const char* prompt, int n, const char* size) {
    static std::string imageUrl;
    std::lock_guard<std::mutex> lock(llmMutex);
    auto it = llmMap.find(handle);
    if (it == llmMap.end() || !model || !prompt || !size)
        return "Error: Invalid instance or parameters.";
    imageUrl = it->second->CreateImage(model, prompt, n, size);
    return imageUrl.c_str();
}

// Destroys an LLMConnection instance.
extern "C" XPLUGIN_API bool LLMDestroy(int handle) {
    std::lock_guard<std::mutex> lock(llmMutex);
    auto it = llmMap.find(handle);
    if (it == llmMap.end()) return false;
    delete it->second;
    llmMap.erase(it);
    return true;
}

//------------------------------------------------------------------------------
// Standalone Getter for the "ToString" Property
//------------------------------------------------------------------------------
extern "C" const char* LLMConnection_ToStringGetter(int handle) {
    std::lock_guard<std::mutex> lock(llmMutex);
    auto it = llmMap.find(handle);
    static std::string s;
    if (it != llmMap.end()) {
        s = it->second->ToString();
        return s.c_str();
    }
    return "";
}

//------------------------------------------------------------------------------
// Class Definition Structures for LLMConnection
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

// Expose properties for APIHost, APIKey, and Organization.
static ClassProperty LLMConnectionProperties[] = {
    { "APIHost", "string", nullptr, (void*)LLMSetAPIHost_Instance },
    { "APIKey", "string", nullptr, (void*)LLMSetConfiguration_Instance },
    { "Organization", "string", nullptr, (void*)LLMSetConfiguration_Instance },
    { "ToString", "string", (void*)LLMConnection_ToStringGetter, nullptr }
};

static ClassEntry LLMConnectionMethods[] = {
    { "CreateCompletion", (void*)LLMCreateCompletion_Instance, 5, {"integer", "string", "string", "integer", "double"}, "string" },
    { "CreateImage", (void*)LLMCreateImage_Instance, 5, {"integer", "string", "string", "integer", "string"}, "string" },
    { "Close", (void*)LLMDestroy, 1, {"integer"}, "boolean" }
};

static ClassConstant LLMConnectionConstants[] = { };

static ClassDefinition LLMConnectionClass = {
    "LLMConnection",
    sizeof(LLMConnection),
    (void*)NewLLMConnection, // Parameterless constructor.
    LLMConnectionProperties,
    sizeof(LLMConnectionProperties) / sizeof(ClassProperty),
    LLMConnectionMethods,
    sizeof(LLMConnectionMethods) / sizeof(ClassEntry),
    LLMConnectionConstants,
    sizeof(LLMConnectionConstants) / sizeof(ClassConstant)
};

extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &LLMConnectionClass;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
#endif
