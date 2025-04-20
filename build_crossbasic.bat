@echo off
setlocal

:: Compile resource file
windres crossbasic.rc -O coff -o crossbasic.res
if %ERRORLEVEL% NEQ 0 (
    echo Resource compilation failed! Check crossbasic.rc for errors.
    exit /b %ERRORLEVEL%
)

:: Compile crossbasic.cpp with metadata
g++ -s -static -m64 -o crossbasic.exe crossbasic.cpp crossbasic.res -Lc:/xojodevkit/x86_64-w64-mingw32/lib/libffix64 -lffi -static-libgcc -static-libstdc++ -O3 -march=native -mtune=native 2> error.log

g++ -s -shared -DBUILD_SHARED -static -m64 -o crossbasic.dll crossbasic.cpp crossbasic.res -Lc:/xojodevkit/x86_64-w64-mingw32/lib/libffix64 -lffi -static-libgcc -static-libstdc++ -O3 -march=native -mtune=native 2> errorlib.log

:: Check if compilation was successful
if %ERRORLEVEL% NEQ 0 (
    echo Compilation failed! Check error.log for details.
    type error.log
    exit /b %ERRORLEVEL%
)

:: Ensure the release directory exists
if not exist release-64 mkdir release-64

:: Move the compiled executable to the release directory
move /Y crossbasic.exe release-64\

:: Move the compiled library to the release directory
move /Y crossbasic.dll release-64\

:: Dump DLL dependencies using objdump
echo DLL dependencies:
objdump -p release-64\crossbasic.exe | findstr /R "DLL"

:: Copy the Scripts folder to the release directory
xcopy /E /I /Y Scripts release-64\Scripts

:: Copy the RunAllScripts Script to the release directory
xcopy /Y runallscripts.bat release-64\

echo CrossBasic Built Successfully.
exit /b 0
