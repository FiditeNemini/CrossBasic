:: Compile the manifest into a resource
windres XTextArea.rc -O coff -o XTextArea.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XTextArea.dll XTextArea.cpp XTextArea.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32