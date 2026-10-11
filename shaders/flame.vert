#version 330 core

// Flames and sparks (src/ENGINE/Smoke.cpp, EmitFlame): one quad per lick,
// standing along the way it points - up from a flame's root, back from a
// spark's head - and turned about that line to face the eye as far as it can.

layout (location = 0) in vec2 corner;       // -0.5..0.5 (the shared particle quad)
layout (location = 1) in vec2 uv;
layout (location = 3) in vec4 iRootLength;  // its root, how long it is now
layout (location = 4) in vec4 iDirWidth;    // the way it points (unit), how wide it is now
layout (location = 5) in vec4 iParams;      // age / life, brightness, temperature, seed
layout (location = 6) in vec4 iParams2;     // spark, unused...

#include "camera.glsl"
#include "smoke_draw.glsl"

out vec2 vUV;             // x across -1..1, y along: 0 at the root .. 1 at the tip
out vec3 vWorld;
flat out vec4 vParams;
flat out float vSpark;
flat out float vWidth;

void main()
{
	vec3 eye = -transpose(mat3(view)) * vec3(view[3]);
	vec3 dir = iDirWidth.xyz;
	vec3 toEye = normalize(eye - iRootLength.xyz);
	vec3 side = cross(dir, toEye);
	float s = length(side);
	// Looked at end on, there is no side to it: the camera's own right will do.
	side = (s > 0.05) ? side / s : vec3(view[0][0], view[1][0], view[2][0]);

	float along = corner.y + 0.5;
	// A flame's quad is a little wider than the flame, so it can waver in it.
	float wide = iDirWidth.w * (iParams2.x > 0.5 ? 1.0 : 1.6);
	vec3 world = iRootLength.xyz + dir * (along * iRootLength.w) + side * (corner.x * wide);
	gl_Position = projection * view * vec4(world, 1.0);

	vUV = vec2(corner.x * 2.0, along);
	vWorld = world;
	vParams = iParams;
	vSpark = iParams2.x;
	vWidth = iDirWidth.w;
}
