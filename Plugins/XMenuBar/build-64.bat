:: Compile the manifest into a resource
windres XMenuBar.rc -O coff -o XMenuBar.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XMenuBar.dll XMenuBar.cpp XMenuBar.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32