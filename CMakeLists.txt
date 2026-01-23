# CMakeLists.txt for GameEngine - Emscripten/WebAssembly Build
#
# This file builds the ENGINE as a static library for Emscripten.
# Game projects should use their own CMakeLists.txt that links against this.
#
# Usage (from engine directory):
#   mkdir build_wasm && cd build_wasm
#   emcmake cmake ..
#   emmake make
#
# This creates: libENGINE.a (static library)

cmake_minimum_required(VERSION 3.13)
project(ENGINE VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Collect all engine source files
file(GLOB_RECURSE ENGINE_SOURCES
    "src/ENGINE/*.cpp"
)

file(GLOB_RECURSE ENGINE_HEADERS
    "src/ENGINE/*.h"
)

if(EMSCRIPTEN)
    message(STATUS "Building ENGINE static library for Emscripten/WebAssembly")

    # Build as static library (Emscripten doesn't support shared libraries well)
    add_library(ENGINE STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

    # Include directories
    target_include_directories(ENGINE PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/ENGINE
    )

    # Define EMSCRIPTEN macro (some code uses this without __)
    target_compile_definitions(ENGINE PUBLIC
        EMSCRIPTEN=1
        MAKEDLL=0
    )

    # Emscripten compiler flags
    target_compile_options(ENGINE PUBLIC
        -sUSE_SDL=2
        -sUSE_SDL_IMAGE=2
        -sUSE_SDL_TTF=2
        -sUSE_SDL_MIXER=2
        -sSDL2_IMAGE_FORMATS=["png","jpg"]
    )

    # Debug vs Release
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_options(ENGINE PRIVATE -g -O0)
    else()
        target_compile_options(ENGINE PRIVATE -O2)
    endif()

    # Output directory
    set_target_properties(ENGINE PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )

    # Install the library and headers (optional)
    install(TARGETS ENGINE ARCHIVE DESTINATION lib)
    install(FILES ${ENGINE_HEADERS} DESTINATION include/ENGINE)

else()
    message(FATAL_ERROR "This CMakeLists.txt is designed for Emscripten builds only. Use Visual Studio for native Windows builds.")
endif()
