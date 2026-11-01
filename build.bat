@echo off
rem Builds the planner, the test runner and the benchmark with g++ (MinGW-w64).
rem Run from the project folder:  build.bat
rem -static bundles the C++ runtime so the .exe files run without MinGW DLLs on PATH.
setlocal
cd /d "%~dp0"
if not exist build mkdir build

set FLAGS=-std=c++17 -O2 -Wall -Wextra -pedantic -static

echo [1/3] route_planner.exe
g++ %FLAGS% -Isrc -o build\route_planner.exe src\main.cpp src\graph.cpp src\router.cpp || goto :fail

echo [2/3] run_tests.exe
g++ %FLAGS% -D_GLIBCXX_ASSERTIONS -Isrc -o build\run_tests.exe tests\test_main.cpp src\graph.cpp src\router.cpp src\reference.cpp || goto :fail

echo [3/3] benchmark.exe
g++ %FLAGS% "-DBENCH_FLAGS=\"%FLAGS%\"" -Isrc -o build\benchmark.exe bench\benchmark.cpp src\graph.cpp src\router.cpp src\reference.cpp || goto :fail

echo Build succeeded.
exit /b 0

:fail
echo Build FAILED.
exit /b 1
