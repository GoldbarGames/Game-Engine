#ifndef HI_Z_H
#define HI_Z_H
#pragma once

// Engine-internal (not exported). A Hi-Z pyramid: a mip chain of a depth
// buffer (R32F) in which each texel holds the FARTHEST depth of the area it
// covers. An object whose nearest depth is behind the farthest depth under its
// screen rectangle - read at the level where that rectangle spans about 2x2
// texels - is hidden. Built by a compute shader (shaders/hiz_build.comp), so
// desktop GL 4.3+ only. Used today by the KINJO_HIZ_STATS measurement
// (docs/RENDERING_NEXT_STEPS.md); the building block occlusion culling would need.

#include "RenderDevice.h"

// Build the pyramid from `depth` (window depth 0..1, width x height). False if
// unsupported or the shader failed; the pyramid is then unusable.
bool BuildHiZ(TextureHandle depth, int width, int height);
TextureHandle HiZTexture();
int HiZLevels();

#endif
