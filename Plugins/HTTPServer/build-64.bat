@echo off
REM =============================================================
REM  build_plugins.bat  –  compile + link static CrossBasic DLLs
REM =============================================================

REM ---------- adjust paths -------------------------------------
set MINGW=C:\crossbasicdevkit
set BOOST=C:\crossbasicdevkit\boost_1_87_0
REM -------------------------------------------------------------

set CXX=%MINGW%\bin\g++.exe
set COMMON=-std=c++17 -O3 -s -static-libgcc -static-libstdc++ -DNDEBUG
set INCL=-I"%BOOST%"
set LIBS_BASE=-lws2_32 -lmswsock -lwinmm -lshlwapi -lz ^
              -lboost_system-mgw14-mt-s-x64-1_87 ^
              -lboost_thread-mgw14-mt-s-x64-1_87

echo.
echo ========= 1/4  HttpRequest.dll ============================
%CXX% %COMMON% %INCL% -shared -o HttpRequest.dll HttpRequest.cpp ^
      -Wl,--out-implib,libHttpRequest.a

echo.
echo ========= 2/4  HttpResponse.dll ===========================
%CXX% %COMMON% %INCL% -shared -o HttpResponse.dll HttpResponse.cpp ^
      -Wl,--out-implib,libHttpResponse.a

echo.
echo ========= 3/4  HttpSession.dll ============================
%CXX% %COMMON% %INCL% -shared -o HttpSession.dll HttpSession.cpp ^
      libHttpRequest.a libHttpResponse.a %LIBS_BASE% ^
      -Wl,--out-implib,libHttpSession.a

echo.
echo ========= 4/4  HttpServer.dll =============================
%CXX% %COMMON% %INCL% -shared -o HttpServer.dll HttpServer.cpp ^
      libHttpSession.a libHttpResponse.a %LIBS_BASE%

echo.
echo All plugins built successfully.
