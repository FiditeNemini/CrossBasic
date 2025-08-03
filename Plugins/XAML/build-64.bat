:: Compile the manifest into a resource
windres XAML.rc -O coff -o XAML.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XAML.dll XAML.cpp XAML.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32