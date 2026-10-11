#ifndef KINJO_SMOKE_DRAW_GLSL
#define KINJO_SMOKE_DRAW_GLSL
// Per-draw values for smoke and steam (src/ENGINE/Smoke.cpp), shared by
// smoke.vert and smoke.frag. 32 bytes.
#include "draw.glsl"

struct DrawData
{
	vec4 uDepth;   // camera near, far; 1 = read the scene depth (soft edges, the shader's own depth test); softness, a fraction of the radius
	vec4 uFade;    // near the eye a puff thins out: from this many radii away, to this many; unused, unused
};
PER_DRAW(DrawData);
#endif
