// DateTime.cpp
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

#include <ctime>
#include <map>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #include <unistd.h>
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

//------------------------------------------------------------------------------
// DateTime Class Declaration
// This class encapsulates a std::tm structure and provides methods to initialize
// a specific date/time, set to current time, retrieve components, and convert to a string.
//------------------------------------------------------------------------------
class DateTime {
public:
    std::tm timeData;

    DateTime() {
        std::memset(&timeData, 0, sizeof(timeData));
        timeData.tm_isdst = -1;
    }

    // Initializes the DateTime instance with the specified date and time.
    void Init(int year, int month, int day, int hour, int minute, int second) {
        timeData.tm_year = year - 1900;  // tm_year is years since 1900
        timeData.tm_mon = month - 1;     // tm_mon is zero-based (0 = January)
        timeData.tm_mday = day;
        timeData.tm_hour = hour;
        timeData.tm_min = minute;
        timeData.tm_sec = second;
        timeData.tm_isdst = -1;          // Let the system determine DST
    }

    // Sets the DateTime instance to the current local date/time.
    void Now() {
        time_t now = time(nullptr);
        std::tm* localTime = localtime(&now);
        if (localTime) {
            timeData = *localTime;
        }
    }

    // Getters for individual date/time components.
    int GetYear()   { return timeData.tm_year + 1900; }
    int GetMonth()  { return timeData.tm_mon + 1; }
    int GetDay()    { return timeData.tm_mday; }
    int GetHour()   { return timeData.tm_hour; }
    int GetMinute() { return timeData.tm_min; }
    int GetSecond() { return timeData.tm_sec; }

    // Returns a string representation of the date/time.
    std::string ToString() {
        char buffer[100];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeData);
        return std::string(buffer);
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for DateTime Objects
//------------------------------------------------------------------------------
static std::map<int, DateTime*> dateTimeInstances;
static int currentHandle = 1; // Unique handle counter

//------------------------------------------------------------------------------
// Constructor: Creates a new DateTime instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    DateTime* dt = new DateTime();
    int handle = currentHandle++;
    dateTimeInstances[handle] = dt;
    return handle;
}

//------------------------------------------------------------------------------
// Instance Methods
//------------------------------------------------------------------------------

// Initializes the DateTime instance with specified parameters.
extern "C" XPLUGIN_API void DateTime_Initialize(int handle, int year, int month, int day, int hour, int minute, int second) {
    if (dateTimeInstances.find(handle) != dateTimeInstances.end()) {
        dateTimeInstances[handle]->Init(year, month, day, hour, minute, second);
    }
}

// Sets the DateTime instance to the current local date/time.
extern "C" XPLUGIN_API void DateTime_Now(int handle) {
    if (dateTimeInstances.find(handle) != dateTimeInstances.end()) {
        dateTimeInstances[handle]->Now();
    }
}

// Retrieves the year.
extern "C" XPLUGIN_API int DateTime_GetYear(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->GetYear();
}

// Retrieves the month (1-12).
extern "C" XPLUGIN_API int DateTime_GetMonth(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->GetMonth();
}

// Retrieves the day (1-31).
extern "C" XPLUGIN_API int DateTime_GetDay(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->GetDay();
}

// Retrieves the hour (0-23).
extern "C" XPLUGIN_API int DateTime_GetHour(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->GetHour();
}

// Retrieves the minute (0-59).
extern "C" XPLUGIN_API int DateTime_GetMinute(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->GetMinute();
}

// Retrieves the second (0-59).
extern "C" XPLUGIN_API int DateTime_GetSecond(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return -1;
    return dateTimeInstances[handle]->GetSecond();
}

// Returns a string representation of the DateTime.
extern "C" XPLUGIN_API const char* DateTime_ToString(int handle) {
    static std::string formattedTime;
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return "Invalid Handle";
    formattedTime = dateTimeInstances[handle]->ToString();
    return formattedTime.c_str();
}

// Destroys a DateTime instance.
extern "C" XPLUGIN_API bool DateTime_Destroy(int handle) {
    if (dateTimeInstances.find(handle) == dateTimeInstances.end()) return false;
    delete dateTimeInstances[handle];
    dateTimeInstances.erase(handle);
    return true;
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
// Define the class properties for DateTime.
// (For this plugin, no properties are exposed; all operations are methods.)
//------------------------------------------------------------------------------
static ClassProperty DateTimeProperties[] = {
    // Optionally, you could expose a read-only "ToString" property.
    { "ToString", "string", (void*)DateTime_ToString, nullptr }
};

//------------------------------------------------------------------------------
// Define the class methods for DateTime.
//------------------------------------------------------------------------------
static ClassEntry DateTimeMethods[] = {
    { "Initialize", (void*)DateTime_Initialize, 7, {"integer", "integer", "integer", "integer", "integer", "integer", "integer"}, "void" },
    { "Now", (void*)DateTime_Now, 1, {"integer"}, "void" },
    { "GetYear", (void*)DateTime_GetYear, 1, {"integer"}, "integer" },
    { "GetMonth", (void*)DateTime_GetMonth, 1, {"integer"}, "integer" },
    { "GetDay", (void*)DateTime_GetDay, 1, {"integer"}, "integer" },
    { "GetHour", (void*)DateTime_GetHour, 1, {"integer"}, "integer" },
    { "GetMinute", (void*)DateTime_GetMinute, 1, {"integer"}, "integer" },
    { "GetSecond", (void*)DateTime_GetSecond, 1, {"integer"}, "integer" },
    { "ToString", (void*)DateTime_ToString, 1, {"integer"}, "string" },
    { "Destroy", (void*)DateTime_Destroy, 1, {"integer"}, "boolean" }
};

// No class constants defined.
static ClassConstant DateTimeConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for DateTime.
//------------------------------------------------------------------------------
static ClassDefinition DateTimeClass = {
    "DateTime",                                        // className
    sizeof(DateTime),                                  // classSize
    (void*)Constructor,                                // constructor (named "Constructor")
    DateTimeProperties,                                // properties
    sizeof(DateTimeProperties) / sizeof(ClassProperty),// propertiesCount
    DateTimeMethods,                                   // methods
    sizeof(DateTimeMethods) / sizeof(ClassEntry),      // methodsCount
    DateTimeConstants,                                 // constants
    sizeof(DateTimeConstants) / sizeof(ClassConstant)  // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &DateTimeClass;
}

#ifdef _WIN32
// Optional DllMain for Windows-specific initialization.
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
