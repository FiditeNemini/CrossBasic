:: Compile the manifest into a resource
windres XButton.rc -O coff -o XButton.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XButton.dll XButton.cpp XButton.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32