@echo off
REM Build script for Windows Client DLL
REM Requires: MinGW-w64 or GCC on Windows

echo ===================================
echo   BUILD CLIENT DLL FOR WINDOWS
echo ===================================
echo.

REM Check if bin directory exists
if not exist bin mkdir bin

echo [BUILD] Compiling client_network.dll...
gcc -shared -o bin\client_network.dll src\client\native\client_network_windows.c src\common\cJSON.c -lws2_32 -I.\src\common

if errorlevel 1 (
    echo.
    echo [ERROR] Build failed!
    echo Make sure you have GCC installed (MinGW-w64 or similar^)
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Client DLL built successfully!
echo File: bin\client_network.dll
echo.
echo ===================================
echo  READY TO USE!
echo ===================================
echo.
echo Now you can run: python src\client\main.py
echo.
pause
