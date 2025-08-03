windres XWindow.rc -O coff -o XWindow.res

g++ -shared -m64 -static -static-libgcc -static-libstdc++ -o XWindow.dll XWindow.cpp XWindow.res -pthread -s -lgdiplus -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32