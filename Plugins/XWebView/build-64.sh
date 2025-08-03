g++ -std=c++17 \
    -shared -fPIC \
    -o libXWebView.so XWebView.cpp \
    `pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.0` \
    -pthread
