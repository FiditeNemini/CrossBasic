:: Compile the manifest into a resource
windres XMenuItem.rc -O coff -o XMenuItem.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XMenuItem.dll XMenuItem.cpp XMenuItem.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32