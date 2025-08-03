:: Compile the manifest into a resource
windres XTextField.rc -O coff -o XTextField.res

g++ -std=c++17 -shared -m64 -static -static-libgcc -static-libstdc++ -o XTextField.dll XTextField.cpp XTextField.res -pthread -s -lcomctl32 -ldwmapi -lgdi32 -luser32 -luxtheme -lole32