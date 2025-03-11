@echo off
echo Compiling Test files...
g++ -o tests.exe tests/test_sha1_base64.cpp -lws2_32 -std=c++17
if %errorlevel% equ 0 (
    echo Running test...
    tests.exe
)