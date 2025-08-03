:: Compile the manifest into a resource
windres XListbox.rc -O coff -o XListbox.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XListbox.dll XListbox.cpp XListbox.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32