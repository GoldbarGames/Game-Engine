#ifndef ENGINE_PATHS_H
#define ENGINE_PATHS_H
#pragma once

#include <string>

// Engine-internal (not exported). Where the engine's own data files live.

// Directory holding the engine-owned shaders (GameEngine/shaders), with a
// trailing '/', or "" if none was found. Searched once, in order:
//   1. the KINJO_ENGINE_SHADERS environment variable
//   2. the engine source tree this file was compiled from (so hot-reload edits
//      the real files during development)
//   3. a "shaders" folder next to the engine DLL (shipped builds; the engine's
//      post-build step copies it there)
// Web builds use "engine/shaders/" (preloaded into the virtual filesystem).
const std::string& EngineShaderDir();

#endif
