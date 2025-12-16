@echo off
REM ============================================================================
REM clean.bat - Complete cleanup script for Windows
REM ============================================================================

echo =========================================================
echo  Movie Booking System - Complete Cleanup (Windows)
echo =========================================================
echo.

echo Cleaning build artifacts...
echo ---------------------------
if exist build (
    echo Removing build directory...
    rmdir /s /q build
    echo   * Removed build\
) else (
    echo   - build\ does not exist (skipping)
)

if exist docs (
    echo Removing docs directory...
    rmdir /s /q docs
    echo   * Removed docs\
) else (
    echo   - docs\ does not exist (skipping)
)

if exist Testing (
    echo Removing Testing directory...
    rmdir /s /q Testing
    echo   * Removed Testing\
) else (
    echo   - Testing\ does not exist (skipping)
)

if exist .cmake (
    echo Removing .cmake directory...
    rmdir /s /q .cmake
    echo   * Removed .cmake\
) else (
    echo   - .cmake\ does not exist (skipping)
)
echo.

echo Cleaning CMake cache files...
echo -----------------------------
if exist CMakeCache.txt (
    del /q CMakeCache.txt
    echo   * Removed CMakeCache.txt
)

if exist cmake_install.cmake (
    del /q cmake_install.cmake
    echo   * Removed cmake_install.cmake
)

if exist CMakeFiles (
    rmdir /s /q CMakeFiles
    echo   * Removed CMakeFiles\
)

if exist Makefile (
    del /q Makefile
    echo   * Removed Makefile
)
echo.

echo Cleaning compiled binaries...
echo -----------------------------
if exist booking_lockfree.exe (
    del /q booking_lockfree.exe
    echo   * Removed booking_lockfree.exe
)

if exist test_lockfree.exe (
    del /q test_lockfree.exe
    echo   * Removed test_lockfree.exe
)

if exist test_overbooking.exe (
    del /q test_overbooking.exe
    echo   * Removed test_overbooking.exe
)

if exist test_two_thread_race.exe (
    del /q test_two_thread_race.exe
    echo   * Removed test_two_thread_race.exe
)

if exist test_scalability.exe (
    del /q test_scalability.exe
    echo   * Removed test_scalability.exe
)
echo.

echo Cleaning library files...
echo ------------------------
if exist booking_lockfree_lib.lib (
    del /q booking_lockfree_lib.lib
    echo   * Removed booking_lockfree_lib.lib
)

if exist *.dll (
    del /q *.dll
    echo   * Removed *.dll files
)
echo.

echo Cleaning object files...
echo -----------------------
for /r %%i in (*.obj) do (
    del /q "%%i" 2>nul
)
echo   * Removed all .obj files
echo.

echo Cleaning Visual Studio files...
echo ------------------------------
if exist .vs (
    rmdir /s /q .vs
    echo   * Removed .vs\
)

if exist *.sln (
    del /q *.sln
    echo   * Removed *.sln files
)

if exist *.vcxproj (
    del /q *.vcxproj
    echo   * Removed *.vcxproj files
)

if exist *.vcxproj.filters (
    del /q *.vcxproj.filters
    echo   * Removed *.vcxproj.filters files
)

if exist *.vcxproj.user (
    del /q *.vcxproj.user
    echo   * Removed *.vcxproj.user files
)
echo.

echo Cleaning editor/IDE files...
echo ---------------------------
if exist .vscode (
    rmdir /s /q .vscode
    echo   * Removed .vscode\
)

if exist .idea (
    rmdir /s /q .idea
    echo   * Removed .idea\
)
echo.

echo Cleaning Docker artifacts (optional)...
echo ---------------------------------------
where docker >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    REM Check if Docker image exists
    docker image inspect booking-lockfree >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        echo Removing Docker image: booking-lockfree
        docker rmi booking-lockfree 2>nul
        if %ERRORLEVEL% EQU 0 (
            echo   * Removed Docker image
        ) else (
            echo   - Failed to remove Docker image (may be in use)
        )
    ) else (
        echo   - Docker image 'booking-lockfree' does not exist
    )
    
    REM Clean dangling images
    echo Cleaning dangling Docker images...
    docker image prune -f >nul 2>nul
    echo   * Cleaned dangling images
) else (
    echo   - Docker not available (skipping Docker cleanup)
)
echo.

echo =========================================================
echo                  CLEANUP COMPLETE
echo =========================================================
echo.
echo All build artifacts, caches, and temporary files removed.
echo The project is now in a clean state.
echo.
echo To rebuild the project, run:
echo   do_all.bat
echo.

pause
