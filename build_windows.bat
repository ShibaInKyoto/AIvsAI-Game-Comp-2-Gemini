@echo off
setlocal enabledelayedexpansion

echo ============================================================================
echo   AETHERNAUT: 3D Sky-Fleet Architect ^& Colossal Leviathan Hunter
echo   Windows 10/11 x64 Native C++20 + OpenGL 3.3 Release Build Script
echo ============================================================================

cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [ERROR] CMake configuration failed. Ensure CMake and Visual Studio C++ tools are installed.
    exit /b 1
)

cmake --build build --config Release --parallel
if %ERRORLEVEL% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [SUCCESS] Built standalone Windows executable: build\bin\Release\Aethernaut.exe
echo Running unit test suite...
ctest --test-dir build -C Release --output-on-failure

echo.
echo Launching Aethernaut...
start "" "build\bin\Release\Aethernaut.exe"
