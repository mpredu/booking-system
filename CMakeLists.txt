cmake_minimum_required(VERSION 3.15)

# ============================================
# Project: Movie Booking System Lock-Free
# ============================================
project(MovieBooking VERSION 1.0 LANGUAGES CXX)

# --------------------------------------------
# General C++ settings
# --------------------------------------------
# C++20 for std::shared_mutex and atomic operations
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)  # avoid compiler-specific extensions

# --------------------------------------------
# Sanitizer Options (cross-platform)
# --------------------------------------------
option(ENABLE_ASAN "Enable AddressSanitizer (memory errors)" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer (race conditions)" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_MSAN "Enable MemorySanitizer (uninitialized memory)" OFF)

# Note: TSAN is mutually exclusive with ASAN/MSAN
if(ENABLE_TSAN AND (ENABLE_ASAN OR ENABLE_MSAN))
    message(FATAL_ERROR "ThreadSanitizer cannot be used with AddressSanitizer or MemorySanitizer")
endif()

# --------------------------------------------
# Cross-platform compiler warnings
# --------------------------------------------
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# --------------------------------------------
# Detect default build type
# Multi-config generators (Visual Studio) ignore CMAKE_BUILD_TYPE
# --------------------------------------------
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose build type (Debug/Release/RelWithDebInfo/MinSizeRel)" FORCE)
endif()

# --------------------------------------------
# Include directories
# --------------------------------------------
include_directories(${PROJECT_SOURCE_DIR}/include)

# --------------------------------------------
# Source files for lock-free library
# --------------------------------------------
set(LIBRARY_SOURCES
    src/SeatBitmask.cpp
    src/BookingService.cpp
)

# --------------------------------------------
# Create static library
# --------------------------------------------
add_library(booking_lockfree_lib STATIC ${LIBRARY_SOURCES})

# --------------------------------------------
# Cross-platform threads support
# --------------------------------------------
find_package(Threads REQUIRED)

# --------------------------------------------
# Sanitizer Configuration (cross-platform)
# --------------------------------------------
function(add_sanitizer_flags target_name)
    if(ENABLE_ASAN)
        message(STATUS "AddressSanitizer enabled for ${target_name}")
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
            target_link_options(${target_name} PRIVATE /fsanitize=address)
        else()
            target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
            target_link_options(${target_name} PRIVATE -fsanitize=address)
        endif()
    endif()

    if(ENABLE_TSAN)
        message(STATUS "ThreadSanitizer enabled for ${target_name}")
        if(MSVC)
            message(WARNING "ThreadSanitizer is not supported on MSVC")
        else()
            target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer -g)
            target_link_options(${target_name} PRIVATE -fsanitize=thread)
        endif()
    endif()

    if(ENABLE_UBSAN)
        message(STATUS "UndefinedBehaviorSanitizer enabled for ${target_name}")
        if(MSVC)
            message(WARNING "UndefinedBehaviorSanitizer is not supported on MSVC")
        else()
            target_compile_options(${target_name} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer -g)
            target_link_options(${target_name} PRIVATE -fsanitize=undefined)
        endif()
    endif()

    if(ENABLE_MSAN)
        message(STATUS "MemorySanitizer enabled for ${target_name}")
        if(MSVC)
            message(WARNING "MemorySanitizer is not supported on MSVC")
        else()
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
                target_compile_options(${target_name} PRIVATE -fsanitize=memory -fno-omit-frame-pointer -g)
                target_link_options(${target_name} PRIVATE -fsanitize=memory)
            else()
                message(WARNING "MemorySanitizer requires Clang compiler")
            endif()
        endif()
    endif()
endfunction()

# Apply sanitizers to library
add_sanitizer_flags(booking_lockfree_lib)

# --------------------------------------------
# Main CLI executable
# --------------------------------------------
add_executable(booking_lockfree src/main.cpp)
target_link_libraries(booking_lockfree PRIVATE booking_lockfree_lib Threads::Threads)
add_sanitizer_flags(booking_lockfree)

# --------------------------------------------
# Test executables
# --------------------------------------------
add_executable(test_lockfree tests/test_lockfree.cpp)
target_link_libraries(test_lockfree PRIVATE booking_lockfree_lib Threads::Threads)
add_sanitizer_flags(test_lockfree)

add_executable(test_overbooking tests/test_overbooking.cpp)
target_link_libraries(test_overbooking PRIVATE booking_lockfree_lib Threads::Threads)
add_sanitizer_flags(test_overbooking)

add_executable(test_scalability tests/test_scalability.cpp)
target_link_libraries(test_scalability PRIVATE booking_lockfree_lib Threads::Threads)
add_sanitizer_flags(test_scalability)

add_executable(test_two_thread_race tests/test_two_thread_race.cpp)
target_link_libraries(test_two_thread_race PRIVATE booking_lockfree_lib Threads::Threads)
add_sanitizer_flags(test_two_thread_race)

# --------------------------------------------
# Enable CTest for cross-platform testing
# --------------------------------------------
enable_testing()
add_test(NAME LockFreeTests COMMAND test_lockfree)
add_test(NAME OverbookingTests COMMAND test_overbooking)
add_test(NAME ScalabilityTests COMMAND test_scalability)
add_test(NAME TwoThreadRace COMMAND test_two_thread_race)

# ============================================
# Build & usage instructions
# ============================================

# 1️⃣ Configure project (Linux/macOS):
# mkdir -p build && cd build
# cmake -S .. -B . -DCMAKE_BUILD_TYPE=Release   # Release build
# cmake -S .. -B . -DCMAKE_BUILD_TYPE=Debug     # Debug build

# 2️⃣ Configure project (Windows, Visual Studio):
# mkdir build && cd build
# cmake -S .. -B . -G "Visual Studio 17 2022"  # multi-config generator
# cmake --build . --config Release              # Release
# cmake --build . --config Debug                # Debug

# 3️⃣ Run tests
# ctest --output-on-failure         # run all tests
# ctest -C Release                  # run tests on Release config (Windows multi-config)

# 4️⃣ Build a specific target
# cmake --build . --target booking_lockfree --config Release
# cmake --build . --target test_lockfree --config Debug

# ============================================
# Optional: define macros based on build type
# ============================================
# Automatically adds NDEBUG in Release
target_compile_definitions(booking_lockfree_lib PUBLIC
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:NDEBUG>
    $<$<CONFIG:RelWithDebInfo>:NDEBUG DEBUG_BUILD>
)

# ============================================
# Sanitizer Usage
# ============================================

# AddressSanitizer (memory errors):
#   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
#   cmake --build . && ./test_lockfree

# ThreadSanitizer (race conditions - Linux/macOS only):
#   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_TSAN=ON ..
#   cmake --build . && ./test_lockfree

# UndefinedBehaviorSanitizer (UB detection):
#   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON ..
#   cmake --build . && ./test_lockfree

# Combine ASAN + UBSAN (safe combination):
#   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
#   cmake --build . && ./test_lockfree

# Or use the automated script(TODO: provide the script):
#   ./sanitizers.sh        # Linux/macOS
#   sanitizers.bat         # Windows
