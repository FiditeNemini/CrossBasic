#!/bin/bash

# Compile crossbasic.cpp using g++
g++ -o crossbasic ./CrossBasic/crossbasic.cpp -lffi -O3 -march=native -mtune=native -flto -m64 2> error.log

# Check if compilation was successful
if [ $? -ne 0 ]; then
    echo "Compilation failed! Check error.log for details."
    cat error.log
    exit 1
fi

# Ensure the release directory exists
mkdir -p release-64

# Move the compiled executable to the release directory
mv -f crossbasic release-64/

# Dump shared library dependencies based on OS and build CrossBasic Embeddable Library
if [[ "$(uname)" == "Darwin" ]]; then
	g++ -o crossbasic.dylib ./CrossBasic/crossbasic.cpp -lffi -O3 -march=native -mtune=native -flto -m64 2> errorlib.log
	mv -f crossbasic.dylib release-64/
    echo "Dylib dependencies:"
    otool -L release-64/crossbasic
elif [[ "$(uname)" == "Linux" ]]; then
	g++ -o crossbasic.so ./CrossBasic/crossbasic.cpp -lffi -O3 -march=native -mtune=native -flto -m64 2> errorlib.log
	mv -f crossbasic.so release-64/
    echo "Shared library dependencies:"
    ldd release-64/crossbasic
else
    echo "Unsupported OS"
fi

# Copy the Scripts folder to the release directory
cp -r Scripts release-64/

# Copy the RunAllScripts Script to the release directory
cp -r runallscripts.sh release-64/

echo "CrossBasic Built Successfully."
exit 0
