/*

  SQLiteDatabase.cpp
  CrossBasic Plugin: SQLiteDatabase                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      4r
 
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
// Build (Linux/macOS):
//    g++ -shared -fPIC -o SQLiteDatabasePlugin.so SQLiteDatabaseClass.cpp -lsqlite3
// Build (Windows):
//    g++ -shared -o SQLiteDatabasePlugin.dll SQLiteDatabaseClass.cpp -lsqlite3

#include <sqlite3.h>
#include <map>
#include <string>
#include <mutex>
#include <atomic>
#include <cstring>

#ifdef _WIN32
  #include <windows.h>
  #define XPLUGIN_API __declspec(dllexport)
#else
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

//------------------------------------------------------------------------------
// SQLiteDatabase Class Declaration
//------------------------------------------------------------------------------
class SQLiteDatabase {
public:
    sqlite3* db;
    std::string lastError;
    std::string DatabaseFile;  // Set externally before calling Open.
    int handle;                // The unique integer handle for this instance.

    // Parameterless constructor.
    SQLiteDatabase() : db(nullptr), handle(0) { }

    // Opens the database using the DatabaseFile property.
    bool Open() {
        if (DatabaseFile.empty()) return false;
        if (db) return true; // already open
        if (sqlite3_open(DatabaseFile.c_str(), &db) != SQLITE_OK) {
            lastError = sqlite3_errmsg(db);
            db = nullptr;
            return false;
        }
        return true;
    }
    
    // Executes a non-query SQL command.
    bool ExecuteSQL(const std::string &sql) {
        if (!db) return false;
        char* errMsg = nullptr;
        int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (errMsg) {
            lastError = errMsg;
            sqlite3_free(errMsg);
        }
        return result == SQLITE_OK;
    }
    
    // Closes the database.
    bool Close() {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
        }
        return true;
    }
    
    // Returns the last error message.
    const char* GetLastError() {
        return lastError.c_str();
    }
    
    // Returns the instance handle.
    int Handle() {
        return handle;
    }
};

//------------------------------------------------------------------------------
// Global Instance Management for SQLiteDatabase Instances
//------------------------------------------------------------------------------
static std::mutex dbMutex;
static std::map<int, SQLiteDatabase*> dbMap;
static std::atomic<int> nextDbHandle(1);

//------------------------------------------------------------------------------
// Exported Functions (Database)
//------------------------------------------------------------------------------

// Constructor: Creates a new SQLiteDatabase instance (parameterless).
extern "C" XPLUGIN_API int OpenDatabase() {
    SQLiteDatabase* dbInstance = new SQLiteDatabase();
    int h = nextDbHandle.fetch_add(1);
    dbInstance->handle = h;
    {
        std::lock_guard<std::mutex> lock(dbMutex);
        dbMap[h] = dbInstance;
    }
    return h;
}

// Sets the DatabaseFile property.
extern "C" XPLUGIN_API void SetDatabaseFile(int dbHandle, const char* file) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    if (it != dbMap.end()) {
        it->second->DatabaseFile = file;
    }
}

// Opens the database.
extern "C" XPLUGIN_API bool Database_Open(int dbHandle) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    if (it == dbMap.end()) return false;
    return it->second->Open();
}

// Executes an SQL command.
extern "C" XPLUGIN_API bool ExecuteSQL(int dbHandle, const char* sql) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    if (it == dbMap.end()) return false;
    return it->second->ExecuteSQL(sql);
}

// Returns the last SQLite error.
extern "C" XPLUGIN_API const char* SQLite_GetLastError(int dbHandle) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    if (it == dbMap.end()) return "Invalid database handle";
    return it->second->GetLastError();
}

// Closes the database.
extern "C" XPLUGIN_API bool CloseDatabase(int dbHandle) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    if (it == dbMap.end()) return false;
    it->second->Close();
    delete it->second;
    dbMap.erase(it);
    return true;
}

// Expose helper to retrieve the underlying sqlite3* pointer.
extern "C" XPLUGIN_API sqlite3* SQLiteDatabase_GetPointer(int dbHandle) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    return (it != dbMap.end()) ? it->second->db : nullptr;
}

// New function: Returns the instance handle.
extern "C" XPLUGIN_API int SQLiteDatabase_Handle(int dbHandle) {
    std::lock_guard<std::mutex> lock(dbMutex);
    auto it = dbMap.find(dbHandle);
    return (it != dbMap.end()) ? it->second->Handle() : 0;
}

//------------------------------------------------------------------------------
// Class Definition Structures for SQLiteDatabase
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

// Expose properties "LastError" (read-only) and "DatabaseFile" (read/write).
static ClassProperty SQLiteDatabaseProperties[] = {
    { "LastError", "string", (void*)SQLite_GetLastError, nullptr },
    { "DatabaseFile", "string", nullptr, (void*)SetDatabaseFile }
};

// Expose a new method "Handle" that returns the instance integer.
static ClassEntry SQLiteDatabaseMethods[] = {
    { "Open", (void*)Database_Open, 1, {"integer"}, "boolean" },
    { "ExecuteSQL", (void*)ExecuteSQL, 2, {"integer", "string"}, "boolean" },
    { "Close", (void*)CloseDatabase, 1, {"integer"}, "boolean" },
    { "Handle", (void*)SQLiteDatabase_Handle, 1, {"integer"}, "integer" }
};

static ClassConstant SQLiteDatabaseConstants[] = {};

static ClassDefinition SQLiteDatabaseClass = {
    "SQLiteDatabase",
    sizeof(SQLiteDatabase),
    (void*)OpenDatabase, // Parameterless constructor.
    SQLiteDatabaseProperties,
    sizeof(SQLiteDatabaseProperties) / sizeof(ClassProperty),
    SQLiteDatabaseMethods,
    sizeof(SQLiteDatabaseMethods) / sizeof(ClassEntry),
    SQLiteDatabaseConstants,
    sizeof(SQLiteDatabaseConstants) / sizeof(ClassConstant)
};

extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &SQLiteDatabaseClass;
}

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}
#endif
