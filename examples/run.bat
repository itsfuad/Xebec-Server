@echo off
echo Compiling Chat Server...
g++ -o server.exe ws.cpp -lws2_32 -std=c++17
if %errorlevel% equ 0 (
    echo Running server...
    server.exe
)