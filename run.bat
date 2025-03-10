@echo off
echo Compiling Xebec Server...
g++ -o server.exe main.cpp -lws2_32 -std=c++17
if %errorlevel% equ 0 (
    echo Running server...
    server.exe
)