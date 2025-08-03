:: Compile the manifest into a resource
windres CrossbasicFramework.rc -O coff -o CrossbasicFramework.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o CrossbasicFramework.dll CrossbasicFramework.cpp CrossbasicFramework.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32