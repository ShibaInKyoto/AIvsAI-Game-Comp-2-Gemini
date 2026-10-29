cmake_minimum_required(VERSION 3.22)
project(ChronoStrider LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime linking for zero-dependency Windows 10/11 portability
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/W4 /WX- /permissive-)
endif()

# Fetch Raylib 5.5 for high-performance native rendering & audio
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
FetchContent_MakeAvailable(raylib)

add_executable(ChronoStrider
    native/src/main.cpp
)

target_include_directories(ChronoStrider PRIVATE native/src)
target_link_libraries(ChronoStrider PRIVATE raylib)

set_target_properties(ChronoStrider PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
