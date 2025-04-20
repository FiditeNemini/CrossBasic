// Shell.cpp
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
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <iostream>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/wait.h>
  #include <fcntl.h>
  #include <signal.h>
#endif

#define XPLUGIN_API __declspec(dllexport)

//------------------------------------------------------------------------------
// Shell Class Declaration
// This class encapsulates shell execution: running a command, capturing output,
// retrieving the exit code, setting a timeout, and killing the process if needed.
//------------------------------------------------------------------------------
class Shell {
public:
    Shell() : exitCode(-1), running(false), timeoutSeconds(5) { }

    // Executes a shell command. Returns true on success.
    bool Execute(const std::string& command) {
        std::lock_guard<std::mutex> lock(shellMutex);
        output.clear();
        exitCode = -1;
        running = true;
#ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        HANDLE hReadPipe, hWritePipe;
        if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) {
            running = false;
            return false;
        }

        STARTUPINFO si = {0};
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdOutput = hWritePipe;
        si.hStdError = hWritePipe;
        si.hStdInput = NULL;

        ZeroMemory(&processInfo, sizeof(processInfo));
        if (!CreateProcess(NULL, const_cast<char*>(command.c_str()),
                           NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &processInfo)) {
            running = false;
            CloseHandle(hWritePipe);
            CloseHandle(hReadPipe);
            return false;
        }
        CloseHandle(hWritePipe);

        char buffer[128];
        DWORD bytesRead;
        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            output += buffer;
        }
        CloseHandle(hReadPipe);

        WaitForSingleObject(processInfo.hProcess, timeoutSeconds * 1000);
        DWORD exitCodeWin;
        GetExitCodeProcess(processInfo.hProcess, &exitCodeWin);
        exitCode = static_cast<int>(exitCodeWin);

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
#else
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            running = false;
            return false;
        }
        pid = fork();
        if (pid == -1) {
            running = false;
            return false;
        }
        if (pid == 0) {
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            execl("/bin/sh", "sh", "-c", command.c_str(), (char*)NULL);
            _exit(127);
        }
        close(pipefd[1]);
        char buffer[128];
        ssize_t n;
        while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            output += buffer;
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
            exitCode = WEXITSTATUS(status);
        else
            exitCode = -1;
#endif
        running = false;
        return true;
    }

    // Sets the execution timeout (in seconds).
    void SetTimeout(int seconds) {
        std::lock_guard<std::mutex> lock(shellMutex);
        timeoutSeconds = (seconds > 0) ? seconds : 5;
    }

    // Returns the captured output from the executed command.
    std::string GetOutput() {
        std::lock_guard<std::mutex> lock(shellMutex);
        return output;
    }

    // Returns the exit code of the executed command.
    int GetExitCode() {
        std::lock_guard<std::mutex> lock(shellMutex);
        return exitCode;
    }

    // Kills the running process.
    bool Kill() {
        std::lock_guard<std::mutex> lock(shellMutex);
        if (!running) return false;
#ifdef _WIN32
        TerminateProcess(processInfo.hProcess, 1);
#else
        kill(pid, SIGKILL);
#endif
        running = false;
        return true;
    }

    // Closes the shell instance.
    void Close() {
        // Nothing additional to do; destruction is handled by Destroy.
    }

private:
    std::string output;
    int exitCode;
    bool running;
    int timeoutSeconds;
#ifdef _WIN32
    PROCESS_INFORMATION processInfo;
#else
    pid_t pid;
#endif
    std::mutex shellMutex;
};

//------------------------------------------------------------------------------
// Global Instance Management for Shell Objects
//------------------------------------------------------------------------------
static std::mutex globalShellMutex;
static std::map<int, Shell*> shellMap;
static int nextShellId = 1;

//------------------------------------------------------------------------------
// Constructor: Creates a new Shell instance and returns its unique handle.
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API int Constructor() {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    int id = nextShellId++;
    Shell* s = new Shell();
    shellMap[id] = s;
    return id;
}

//------------------------------------------------------------------------------
// Exported Functions (wrappers for class methods)
//------------------------------------------------------------------------------
extern "C" XPLUGIN_API bool Shell_Execute(int id, const char* command) {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    auto it = shellMap.find(id);
    if (it == shellMap.end()) return false;
    return it->second->Execute(command);
}

extern "C" XPLUGIN_API void Shell_SetTimeout(int id, int seconds) {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    auto it = shellMap.find(id);
    if (it != shellMap.end())
        it->second->SetTimeout(seconds);
}

extern "C" XPLUGIN_API const char* Shell_Result(int id) {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    auto it = shellMap.find(id);
    if (it == shellMap.end()) return "";
    static std::string result;
    result = it->second->GetOutput();
    return result.c_str();
}

extern "C" XPLUGIN_API int Shell_ExitCode(int id) {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    auto it = shellMap.find(id);
    if (it == shellMap.end()) return -1;
    return it->second->GetExitCode();
}

extern "C" XPLUGIN_API bool Shell_Kill(int id) {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    auto it = shellMap.find(id);
    if (it == shellMap.end()) return false;
    return it->second->Kill();
}

extern "C" XPLUGIN_API bool Shell_Destroy(int id) {
    std::lock_guard<std::mutex> lock(globalShellMutex);
    auto it = shellMap.find(id);
    if (it == shellMap.end()) return false;
    delete it->second;
    shellMap.erase(it);
    return true;
}

//------------------------------------------------------------------------------
// Class Definition Structures for the Shell class
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
// Define the class properties for Shell.
// Expose read-only properties "Result" and "ExitCode".
static ClassProperty ShellProperties[] = {
    { "Result", "string", (void*)Shell_Result, nullptr },
    { "ExitCode", "integer", (void*)Shell_ExitCode, nullptr }
};

//------------------------------------------------------------------------------
// Define the class methods for Shell.
static ClassEntry ShellMethods[] = {
    { "Execute", (void*)Shell_Execute, 2, {"integer", "string"}, "boolean" },
    { "SetTimeout", (void*)Shell_SetTimeout, 2, {"integer", "integer"}, "void" },
    { "Kill", (void*)Shell_Kill, 1, {"integer"}, "boolean" },
    { "Close", (void*)Shell_Destroy, 1, {"integer"}, "boolean" }
};

// No class constants defined.
static ClassConstant ShellConstants[] = {};

//------------------------------------------------------------------------------
// Complete class definition for Shell.
static ClassDefinition ShellClass = {
    "Shell",                                      // className
    sizeof(Shell),                                // classSize
    (void*)Constructor,                           // constructor
    ShellProperties,                              // properties
    sizeof(ShellProperties) / sizeof(ClassProperty), // propertiesCount
    ShellMethods,                                 // methods
    sizeof(ShellMethods) / sizeof(ClassEntry),    // methodsCount
    ShellConstants,                               // constants
    sizeof(ShellConstants) / sizeof(ClassConstant) // constantsCount
};

//------------------------------------------------------------------------------
// Exported function to return the class definition.
extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() {
    return &ShellClass;
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
