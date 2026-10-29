@echo off
echo =======================================================
echo   ChronoStrider: Quantum Breach - Windows 10/11 Native Build
echo =======================================================
echo Checking Visual Studio 2022 / C++ Build Tools...

where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] CMake not found. Please install CMake 3.22+ from https://cmake.org/download/
    pause
    exit /b 1
)

echo Configuring CMake for Windows x64 (MSVC Statically Linked)...
cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    pause
    exit /b 1
)

echo Building ChronoStrider.exe in Release mode...
cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    pause
    exit /b 1
)

echo =======================================================
echo   BUILD SUCCESSFUL!
echo   Executable location: build\bin\Release\ChronoStrider.exe
echo =======================================================
build\bin\Release\ChronoStrider.exe
pause
