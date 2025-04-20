# apt install build-essential libboost-system-dev libboost-thread-dev zlib1g-dev
g++ server.cpp -O3 -std=c++17 -s -static-libgcc -static-libstdc++ -lboost_system -lboost_thread -pthread -lz -o server