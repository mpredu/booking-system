@echo off
REM ============================================================================
REM check_compiler.bat - Check for C++20 compiler with barrier support (Windows)
REM ============================================================================

setlocal enabledelayedexpansion

echo Checking C++20 support...

set CXX_COMPILER=
set CXX_FOUND=0

REM Create test file
set TEST_FILE=%TEMP%\test_barrier_%RANDOM%.cpp
echo #include ^<barrier^> > "%TEST_FILE%"
echo int main() { return 0; } >> "%TEST_FILE%"

REM Try to find MSVC (cl.exe)
where cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Testing MSVC cl.exe...
    cl /std:c++20 /c "%TEST_FILE%" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set CXX_COMPILER=cl.exe
        set CXX_FOUND=1
        echo [32m✓ Found MSVC with C++20 support[0m
        goto :found
    ) else (
        echo [31m✗ MSVC does NOT support C++20 barrier[0m
    )
)

REM Try clang-cl (if installed)
where clang-cl.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Testing clang-cl.exe...
    clang-cl /std:c++20 /c "%TEST_FILE%" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set CXX_COMPILER=clang-cl.exe
        set CXX_FOUND=1
        echo [32m✓ Found clang-cl with C++20 support[0m
        goto :found
    ) else (
        echo [31m✗ clang-cl does NOT support C++20 barrier[0m
    )
)

REM Try clang++ (MinGW/MSYS2)
where clang++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Testing clang++.exe...
    clang++ -std=c++20 -stdlib=libc++ -c "%TEST_FILE%" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set CXX_COMPILER=clang++.exe
        set CXX_FOUND=1
        echo [32m✓ Found clang++ with C++20 support (libc++)[0m
        goto :found
    ) else (
        echo [31m✗ clang++ does NOT support C++20 barrier[0m
    )
)

REM Try g++ (MinGW/MSYS2)
where g++.exe >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Testing g++.exe...
    g++ -std=c++20 -c "%TEST_FILE%" >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set CXX_COMPILER=g++.exe
        set CXX_FOUND=1
        echo [32m✓ Found g++ with C++20 support[0m
        goto :found
    ) else (
        echo [31m✗ g++ does NOT support C++20 barrier[0m
    )
)

:found
del "%TEST_FILE%" >nul 2>&1

if %CXX_FOUND% EQU 0 (
    echo.
    echo [31m✗ No C++20 compiler with ^<barrier^> support found[0m
    echo.
    echo Please install a compatible compiler:
    echo.
    echo For Visual Studio 2022:
    echo   - Install Visual Studio 2022 with C++ Desktop Development
    echo   - Ensure MSVC v143 or newer is installed
    echo.
    echo For MSYS2/MinGW:
    echo   pacman -S mingw-w64-x86_64-gcc
    echo   or
    echo   pacman -S mingw-w64-x86_64-clang
    echo.
    exit /b 1
)

echo.
echo Using compiler: %CXX_COMPILER%
endlocal
exit /b 0
