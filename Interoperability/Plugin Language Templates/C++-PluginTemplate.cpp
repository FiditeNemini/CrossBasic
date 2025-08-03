// PluginTemplate.cpp
// Build (Windows): g++ -shared -fPIC -o PluginTemplate.dll PluginTemplate.cpp 
// Build (macOS/Linux): g++ -shared -fPIC -o PluginTemplate.dylib/so PluginTemplate.cpp

#include <cstdio>
#include <string>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#include <unistd.h> // for usleep on Unix
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

    // Type definition for a plugin function that takes two integers and returns an integer.
    typedef int (*PluginFuncInt)(int, int);

    // Structure representing a plugin function entry.
    typedef struct PluginEntry {
        const char* name;                 // Name to be used by the bytecode interpreter.
        void* funcPtr;                    // Pointer to the exported function.
        int arity;                        // Number of parameters the function accepts.
        const char* paramTypes[10];       // Parameter datatypes.
        const char* retType;              // Return datatype.
    } PluginEntry;

    // Sample exported function: adds two integers.
    XPLUGIN_API int addtwonumbers(int a, int b) {
        return a + b;
    }

    // Sample exported function: Says Hello to the user.
    XPLUGIN_API const char* sayhello(const char* name) {
        static std::string greeting;
        greeting = "Hello, " + std::string(name);
        return greeting.c_str();
    }

    // Factorial function
    XPLUGIN_API int factorial(int n) {
        if (n <= 1)
            return 1;
        else
            return n * factorial(n - 1);
    }

    // Fibonacci function
    XPLUGIN_API int Fibonacci(int n2) {
        if (n2 <= 0)
            return 0;
        else if (n2 == 1)
            return 1;
        else
            return Fibonacci(n2 - 1) + Fibonacci(n2 - 2);
    }

    // Cross platform Beep function: Beep(Frequency as Integer, Duration as Integer) as Boolean
    XPLUGIN_API bool XBeep(int frequency, int duration) {
    #ifdef _WIN32
        return ::Beep(frequency, duration) != 0;
    #else
        std::printf("\a");
        std::fflush(stdout);
        usleep(duration * 1000); // usleep takes microseconds
        return true;
    #endif
    }

    // Cross platform Sleep function (renamed to PluginSleep to avoid naming conflicts):
    // Sleep(Milliseconds) as Boolean
    XPLUGIN_API bool PluginSleep(int ms) {
    #ifdef _WIN32
        Sleep(ms);
    #else
        usleep(ms * 1000);
    #endif
        return true;
    }
	
	// **DoEvents() Function** - Allows UI events to be processed
    XPLUGIN_API void DoEvents() {
    #ifdef _WIN32
        // Process pending UI messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Allow Windows to handle UI input
        MsgWaitForMultipleObjectsEx(0, nullptr, 0, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
    #else
        // Yield CPU to allow other tasks to run
        sched_yield();
    #endif
    }

    // Static array of plugin method entries; add more entries as needed.
    static PluginEntry pluginEntries[] = {
        { "AddTwoNumbers", (void*)addtwonumbers, 2, {"double", "double"}, "double" },
        { "SayHello", (void*)sayhello, 1, {"string"}, "string" },
        { "Factorial", (void*)factorial, 1, {"integer"}, "integer" },
        { "Fibonacci", (void*)Fibonacci, 1, {"integer"}, "integer" },
        { "Beep", (void*)XBeep, 2, {"integer", "integer"}, "boolean" },
        { "Sleep", (void*)PluginSleep, 1, {"integer"}, "boolean" },
        { "DoEvents", (void*)DoEvents, 0, {}, "boolean" } // Added DoEvents function
    };

    // Exported function to retrieve the plugin entries.
    // 'count' will be set to the number of entries.
    XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
        if (count) {
            *count = sizeof(pluginEntries) / sizeof(PluginEntry);
        }
        return pluginEntries;
    }

} // extern "C"

#ifdef _WIN32
// Optional: DllMain for Windows-specific initialization.
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif
