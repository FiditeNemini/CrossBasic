:: Compile the manifest into a resource
windres XScreen.rc -O coff -o XScreen.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XScreen.dll XScreen.cpp XScreen.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32