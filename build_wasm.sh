#!/bin/bash
# Build script for Emscripten/WebAssembly - ENGINE LIBRARY
#
# NOTE: This builds the ENGINE as a static library (libENGINE.a).
#       To build a runnable game, use the CMakeLists.txt in your GAME project.
#       See EMSCRIPTEN_BUILD.txt for instructions.
#
# Prerequisites:
#   1. Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html
#   2. Source emsdk_env.sh to set up environment variables
#
# Usage:
#   ./build_wasm.sh [debug|release]

set -e

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo "ERROR: Emscripten compiler (emcc) not found in PATH."
    echo ""
    echo "Please install the Emscripten SDK and source emsdk_env.sh first:"
    echo "  1. git clone https://github.com/emscripten-core/emsdk.git"
    echo "  2. cd emsdk"
    echo "  3. ./emsdk install latest"
    echo "  4. ./emsdk activate latest"
    echo "  5. source ./emsdk_env.sh"
    echo ""
    exit 1
fi

# Parse build type argument
BUILD_TYPE="Release"
if [ "$1" = "debug" ] || [ "$1" = "Debug" ]; then
    BUILD_TYPE="Debug"
fi

echo "Building GameEngine for WebAssembly ($BUILD_TYPE)..."
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Create build directory
BUILD_DIR="build_wasm"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Run CMake with Emscripten toolchain
echo "Running CMake..."
emcmake cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo ""
echo "Building..."
emmake cmake --build . --config "$BUILD_TYPE"

cd ..

echo ""
echo "Build complete!"
echo "Output: $BUILD_DIR/lib/libENGINE.a"
echo ""
echo "NOTE: This is just the engine library. To build a runnable game:"
echo "  1. Copy CMakeLists_Game_Template.txt to your game project"
echo "  2. Configure it for your game"
echo "  3. Build from your game directory"
echo ""
echo "See EMSCRIPTEN_BUILD.txt for detailed instructions."
