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
/*
───────────────────────────────────────────────────────────────────────────────
 Build commands
───────────────────────────────────────────────────────────────────────────────
 Windows (MinGW‑w64)
   g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ ^
       -o Shell.dll Shell.cpp -pthread

 Linux
   g++ -std=c++17 -shared -fPIC -o libShell.so Shell.cpp -pthread

 macOS
   g++ -std=c++17 -dynamiclib -o libShell.dylib Shell.cpp -pthread
───────────────────────────────────────────────────────────────────────────────
*/

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <iostream>
#include <cstdlib>
#include <cstring>   // ← strdup / _strdup

#ifdef _WIN32
  #include <windows.h>
  #define strdup  _strdup
  #define XPLUGIN_API __declspec(dllexport)
#else
  #include <unistd.h>
  #include <sys/wait.h>
  #include <fcntl.h>
  #include <signal.h>
  #define XPLUGIN_API __attribute__((visibility("default")))
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  Shell class – encapsulates a single shell/process invocation
// ─────────────────────────────────────────────────────────────────────────────
class Shell {
public:
    Shell() : exitCode(-1), running(false), timeoutSeconds(5) {}

    // Execute a command; capture stdout+stderr. Returns true on success.
    bool Execute(const std::string& command)
    {
        std::lock_guard<std::mutex> lock(shellMutex);
        output.clear();
        exitCode = -1;
        running  = true;

#ifdef _WIN32
        SECURITY_ATTRIBUTES saAttr{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

        HANDLE hRead = nullptr, hWrite = nullptr;
        if (!CreatePipe(&hRead, &hWrite, &saAttr, 0)) {
            running = false;
            return false;
        }

        STARTUPINFOA si{};
        si.cb          = sizeof(STARTUPINFOA);
        si.dwFlags     = STARTF_USESTDHANDLES;
        si.hStdOutput  = hWrite;
        si.hStdError   = hWrite;

        ZeroMemory(&procInfo, sizeof(procInfo));

        // CreateProcess wants a *modifiable* char*
        std::vector<char> cmdBuf(command.begin(), command.end());
        cmdBuf.push_back('\0');

        if (!CreateProcessA(nullptr, cmdBuf.data(),
                            nullptr, nullptr, TRUE,
                            CREATE_NO_WINDOW, nullptr, nullptr,
                            &si, &procInfo))
        {
            running = false;
            CloseHandle(hWrite);
            CloseHandle(hRead);
            return false;
        }

        CloseHandle(hWrite);               // parent only reads

        char buf[256];
        DWORD nRead = 0;
        while (ReadFile(hRead, buf, sizeof(buf)-1, &nRead, nullptr) && nRead)
        {
            buf[nRead] = '\0';
            output += buf;
        }
        CloseHandle(hRead);

        WaitForSingleObject(procInfo.hProcess, timeoutSeconds * 1000);

        DWORD codeWin = 0;
        GetExitCodeProcess(procInfo.hProcess, &codeWin);
        exitCode = static_cast<int>(codeWin);

        CloseHandle(procInfo.hProcess);
        CloseHandle(procInfo.hThread);

#else   // ───────────── POSIX ─────────────
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

        if (pid == 0) {                    // child
            close(pipefd[0]);              // close read end
            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[1]);
            execl("/bin/sh", "sh", "-c", command.c_str(), (char*)nullptr);
            _exit(127);                    // exec failed
        }

        // parent
        close(pipefd[1]);                  // close write end

        char buf[256];
        ssize_t n;
        while ((n = read(pipefd[0], buf, sizeof(buf)-1)) > 0)
        {
            buf[n] = '\0';
            output += buf;
        }
        close(pipefd[0]);

        int status = 0;
        waitpid(pid, &status, 0);
        exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
        running = false;
        return true;
    }

    void SetTimeout(int seconds)
    {
        std::lock_guard<std::mutex> lock(shellMutex);
        timeoutSeconds = (seconds > 0) ? seconds : 5;
    }

    std::string GetOutput()
    {
        std::lock_guard<std::mutex> lock(shellMutex);
        return output;
    }

    int GetExitCode()
    {
        std::lock_guard<std::mutex> lock(shellMutex);
        return exitCode;
    }

    bool Kill()
    {
        std::lock_guard<std::mutex> lock(shellMutex);
        if (!running) return false;
#ifdef _WIN32
        TerminateProcess(procInfo.hProcess, 1);
#else
        ::kill(pid, SIGKILL);
#endif
        running = false;
        return true;
    }

    void Close() { /* placeholder for symmetry; real cleanup in Destroy */ }

private:
    std::string output;
    int         exitCode;
    bool        running;
    int         timeoutSeconds;

#ifdef _WIN32
    PROCESS_INFORMATION procInfo{};
#else
    pid_t pid{};
#endif
    std::mutex shellMutex;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Global instance management
// ─────────────────────────────────────────────────────────────────────────────
static std::mutex         g_mutex;
static std::map<int, Shell*> g_shells;
static int                g_nextId = 1;

// ── Constructor ──────────────────────────────────────────────────────────────
extern "C" XPLUGIN_API int Constructor()
{
    std::lock_guard<std::mutex> lk(g_mutex);
    int id = g_nextId++;
    g_shells[id] = new Shell;
    return id;
}

// ── Wrapper helpers ──────────────────────────────────────────────────────────
static Shell* getShell(int id)
{
    auto it = g_shells.find(id);
    return (it != g_shells.end()) ? it->second : nullptr;
}

extern "C" XPLUGIN_API bool Shell_Execute(int id, const char* cmd)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    Shell* s = getShell(id);
    return s ? s->Execute(cmd ? cmd : "") : false;
}

extern "C" XPLUGIN_API void Shell_SetTimeout(int id, int seconds)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    if (Shell* s = getShell(id)) s->SetTimeout(seconds);
}

extern "C" XPLUGIN_API const char* Shell_Result(int id)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    static std::string tmp;
    if (Shell* s = getShell(id)) tmp = s->GetOutput(); else tmp.clear();
    return tmp.c_str();        // valid until next call
}

extern "C" XPLUGIN_API int Shell_ExitCode(int id)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    if (Shell* s = getShell(id)) return s->GetExitCode();
    return -1;
}

extern "C" XPLUGIN_API bool Shell_Kill(int id)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    if (Shell* s = getShell(id)) return s->Kill();
    return false;
}

extern "C" XPLUGIN_API bool Shell_Destroy(int id)
{
    std::lock_guard<std::mutex> lk(g_mutex);
    auto it = g_shells.find(id);
    if (it == g_shells.end()) return false;
    delete it->second;
    g_shells.erase(it);
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  VM Class‑definition scaffolding
// ─────────────────────────────────────────────────────────────────────────────
typedef struct { const char* name; const char* type; void* getter; void* setter; } ClassProperty;
typedef struct { const char* name; void* funcPtr; int arity; const char* paramTypes[10]; const char* retType; } ClassEntry;
typedef struct { const char* declaration; } ClassConstant;
typedef struct {
    const char*     className;
    size_t          classSize;
    void*           constructor;
    ClassProperty*  properties;
    size_t          propertiesCount;
    ClassEntry*     methods;
    size_t          methodsCount;
    ClassConstant*  constants;
    size_t          constantsCount;
} ClassDefinition;

// Properties
static ClassProperty props[] = {
    { "Result",   "string",  (void*)Shell_Result,  nullptr },
    { "ExitCode", "integer", (void*)Shell_ExitCode, nullptr }
};

// Methods
static ClassEntry methods[] = {
    { "Execute",    (void*)Shell_Execute,    2, {"integer","string"},   "boolean" },
    { "SetTimeout", (void*)Shell_SetTimeout, 2, {"integer","integer"},  "void"    },
    { "Kill",       (void*)Shell_Kill,       1, {"integer"},            "boolean" },
    { "Close",      (void*)Shell_Destroy,    1, {"integer"},            "boolean" }
};

// (no constants)
static ClassConstant consts[] = {};

static ClassDefinition shellClass = {
    "Shell",
    sizeof(Shell),
    (void*)Constructor,
    props,   sizeof(props)/sizeof(props[0]),
    methods, sizeof(methods)/sizeof(methods[0]),
    consts,  sizeof(consts)/sizeof(consts[0])
};

extern "C" XPLUGIN_API ClassDefinition* GetClassDefinition() { return &shellClass; }

// ─────────────────────────────────────────────────────────────────────────────
//  Windows DLL entry‑point (optional cleanup could be added)
// ─────────────────────────────────────────────────────────────────────────────

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
