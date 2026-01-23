@echo off
REM Build script for Emscripten/WebAssembly - ENGINE LIBRARY
REM
REM NOTE: This builds the ENGINE as a static library (libENGINE.a).
REM       To build a runnable game, use the CMakeLists.txt in your GAME project.
REM       See EMSCRIPTEN_BUILD.txt for instructions.
REM
REM Prerequisites:
REM   1. Install Emscripten SDK: https://emscripten.org/docs/getting_started/downloads.html
REM   2. Run emsdk_env.bat to set up environment variables
REM
REM Usage:
REM   build_wasm.bat [debug|release]

REM Check if emcc is available
emcc --version >nul 2>nul
if %ERRORLEVEL% neq 0 (
    echo ERROR: Emscripten compiler ^(emcc^) not found in PATH.
    echo.
    echo Please install the Emscripten SDK and run emsdk_env.bat first:
    echo   1. git clone https://github.com/emscripten-core/emsdk.git
    echo   2. cd emsdk
    echo   3. emsdk install latest
    echo   4. emsdk activate latest
    echo   5. emsdk_env.bat
    echo.
    exit /b 1
)

setlocal EnableDelayedExpansion

REM Parse build type argument
set BUILD_TYPE=Release
if /i "%1"=="debug" set BUILD_TYPE=Debug
if /i "%1"=="release" set BUILD_TYPE=Release

echo Building GameEngine for WebAssembly ^(%BUILD_TYPE%^)...
echo.

REM Create build directory
set BUILD_DIR=build_wasm
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

REM Run CMake with Emscripten toolchain
echo Running CMake...
call emcmake cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed.
    cd ..
    exit /b 1
)

echo.
echo Building...
call emmake cmake --build . --config %BUILD_TYPE%
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed.
    cd ..
    exit /b 1
)

cd ..

echo.
echo Build complete!
echo Output: %BUILD_DIR%\lib\libENGINE.a
echo.
echo NOTE: This is just the engine library. To build a runnable game:
echo   1. Copy CMakeLists_Game_Template.txt to your game project
echo   2. Configure it for your game
echo   3. Build from your game directory
echo.
echo See EMSCRIPTEN_BUILD.txt for detailed instructions.
