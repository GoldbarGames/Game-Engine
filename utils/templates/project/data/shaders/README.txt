Game shader folder: overrides and game-specific effects only.

The engine's own shaders (sprite/gui/tile/color/instanced, the Scene3D set,
shadows, weather, flash) live in GameEngine/shaders and are used by every game
automatically - do not copy them here.

To customize one for this game, put a file with the same name here; the game's
copy wins over the engine's (e.g. MazeMiner keeps its own shader.vert/frag).
Shaders listed in data/config/shaders.dat are looked up here first, then in
the engine folder. `#include "name.glsl"` resolves the same way.
