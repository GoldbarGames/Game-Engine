#ifndef KINJO_SPRITE_DRAW_GLSL
#define KINJO_SPRITE_DRAW_GLSL
// Per-draw values for the sprite family (gui, shader, tile, color, instanced,
// the framebuffer composite): what Sprite::Render / Renderer::FlushBatch set
// for every sprite. 112 bytes - within the 128-byte push-constant budget.
#include "draw.glsl"

struct DrawData
{
	mat4  model;
	vec2  texFrame;      // size of one animation frame, in UV
	vec2  texOffset;     // that frame's UV origin
	vec4  spriteColor;   // tint
	float lightRatio;    // 2D light falloff (below 0.1 = "cave mode": point lights only)
	float time;          // renderer time, ms (effect shaders)
	float freq;          // effect frequency (effect shaders)
	float emissive;      // 1 = unlit
};
PER_DRAW(DrawData);
#endif
