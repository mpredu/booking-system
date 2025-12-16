@echo off
REM ============================================================================
REM do_all.bat - Complete build, test, and documentation script for Windows
REM ============================================================================

echo =========================================================
echo  Movie Booking System - Complete Build Script (Windows)
echo =========================================================
echo.

REM Check if CMake is available
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake not found in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Check if Docker is available (optional)
where docker >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set DOCKER_AVAILABLE=1
    echo Docker detected - will build and test Docker image
) else (
    set DOCKER_AVAILABLE=0
    echo Docker not detected - skipping Docker build
)

REM Check if Doxygen is available (optional)
where doxygen >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    set DOXYGEN_AVAILABLE=1
    echo Doxygen detected - will generate documentation
) else (
    set DOXYGEN_AVAILABLE=0
    echo Doxygen not detected - skipping documentation generation
)

echo.
echo =========================================================
echo Step 1: Clean previous build
echo =========================================================
if exist build rmdir /s /q build
if exist docs rmdir /s /q docs
if exist Testing rmdir /s /q Testing
echo Clean complete.
echo.

echo =========================================================
echo Step 2: Create build directory and configure
echo =========================================================
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022"
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    cd ..
    pause
    exit /b 1
)
cd ..
echo Configuration complete.
echo.

echo =========================================================
echo Step 3: Build project (Release configuration)
echo =========================================================
cmake --build build --config Release
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)
echo Build complete.
echo.

echo =========================================================
echo Step 4: Run tests
echo =========================================================
cd build

echo Running test_lockfree...
Release\test_lockfree.exe
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: test_lockfree failed
)
echo.

echo Running test_overbooking...
Release\test_overbooking.exe
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: test_overbooking failed
)
echo.

echo Running test_two_thread_race...
Release\test_two_thread_race.exe
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: test_two_thread_race failed
)
echo.

echo Running test_scalability...
Release\test_scalability.exe
if %ERRORLEVEL% NEQ 0 (
    echo WARNING: test_scalability failed
)
echo.

cd ..
echo All tests completed.
echo.

REM Generate documentation if Doxygen is available
if %DOXYGEN_AVAILABLE% EQU 1 (
    echo =========================================================
    echo Step 5: Generate documentation with Doxygen
    echo =========================================================
    doxygen Doxyfile
    if %ERRORLEVEL% EQU 0 (
        echo Documentation generated in docs\html\index.html
    ) else (
        echo WARNING: Doxygen generation failed
    )
    echo.
) else (
    echo =========================================================
    echo Step 5: Skip documentation (Doxygen not available)
    echo =========================================================
    echo Install Doxygen from https://www.doxygen.nl/download.html
    echo.
)

REM Docker build and test if available
if %DOCKER_AVAILABLE% EQU 1 (
    echo =========================================================
    echo Step 6: Build and test Docker image
    echo =========================================================
    
    echo Building Docker image...
    docker build -t booking-lockfree .
    if %ERRORLEVEL% NEQ 0 (
        echo WARNING: Docker build failed
        goto :skip_docker
    )
    echo.
    
    echo Running Docker tests...
    echo - test_lockfree
    docker run --rm booking-lockfree test_lockfree
    echo.
    
    echo - test_overbooking
    docker run --rm booking-lockfree test_overbooking
    echo.
    
    echo - test_two_thread_race
    docker run --rm booking-lockfree test_two_thread_race
    echo.
    
    echo - test_scalability
    docker run --rm booking-lockfree test_scalability
    echo.
    
    :skip_docker
) else (
    echo =========================================================
    echo Step 6: Skip Docker (Docker not available)
    echo =========================================================
    echo Install Docker from https://www.docker.com/products/docker-desktop
    echo.
)

echo =========================================================
echo                    BUILD COMPLETE
echo =========================================================
echo.
echo Executables are in: build\Release\
echo - booking_lockfree.exe (CLI application)
echo - test_lockfree.exe
echo - test_overbooking.exe
echo - test_two_thread_race.exe
echo - test_scalability.exe
echo.

if %DOXYGEN_AVAILABLE% EQU 1 (
    echo Documentation: docs\html\index.html
    echo.
)

echo To run the CLI application:
echo   cd build\Release
echo   booking_lockfree.exe
echo.

pause
