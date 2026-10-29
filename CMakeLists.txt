cmake_minimum_required(VERSION 3.22)
project(Aethernaut VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static MSVC / MinGW runtime linking on Windows 10/11 so Aethernaut.exe runs on any clean PC
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX WIN32_LEAN_AND_MEAN)
elseif(MINGW)
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

# Fetch Raylib 5.5
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

if(EXISTS "/tmp/deps/raylib/CMakeLists.txt")
    set(FETCHCONTENT_SOURCE_DIR_RAYLIB "/tmp/deps/raylib" CACHE PATH "" FORCE)
endif()

FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc /MP)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-missing-field-initializers)
endif()

# -----------------------------------------------------------------------------
# Core & Deterministic 3D Physics Simulation Library (Headless-capable)
# -----------------------------------------------------------------------------
add_library(aethernaut_sim STATIC
    src/core/Math3D.cpp
    src/core/SpatialHash3D.cpp
    src/sim/BlockRegistry.cpp
    src/sim/Airship.cpp
    src/sim/ShipBlueprints.cpp
    src/sim/Leviathan.cpp
    src/sim/Ballistics.cpp
    src/sim/Archipelago.cpp
    src/sim/AIController.cpp
    src/sim/World.cpp
)

target_include_directories(aethernaut_sim PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# -----------------------------------------------------------------------------
# Main 3D Game Executable (Aethernaut)
# -----------------------------------------------------------------------------
add_executable(Aethernaut
    src/main.cpp
    src/core/AppConfig.cpp
    src/render/ShaderPipeline.cpp
    src/render/MeshGen.cpp
    src/render/WorldRenderer.cpp
    src/audio/SynthAudio.cpp
    src/ui/DrydockEditor.cpp
    src/ui/HudRenderer.cpp
)

target_include_directories(Aethernaut PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Aethernaut PRIVATE
    aethernaut_sim
    raylib
)

if(WIN32)
    target_link_libraries(Aethernaut PRIVATE winmm gdi32 opengl32)
endif()

set_target_properties(Aethernaut PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy shaders & assets next to executable on post-build
add_custom_command(TARGET Aethernaut POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:Aethernaut>/assets"
    COMMENT "Copying GLSL shaders and assets to output directory"
)

# -----------------------------------------------------------------------------
# Automated Unit & Physics Verification Tests
# -----------------------------------------------------------------------------
enable_testing()

add_executable(aethernaut_tests
    tests/test_main.cpp
)
target_link_libraries(aethernaut_tests PRIVATE aethernaut_sim)
set_target_properties(aethernaut_tests PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
add_test(NAME PhysicsAndSimulationTests COMMAND aethernaut_tests)

# -----------------------------------------------------------------------------
# Headless Simulation & Performance Benchmark Tool
# -----------------------------------------------------------------------------
add_executable(aethernaut_benchmark
    tools/sim_benchmark.cpp
)
target_link_libraries(aethernaut_benchmark PRIVATE aethernaut_sim)
set_target_properties(aethernaut_benchmark PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
