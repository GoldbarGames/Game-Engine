#version 330 core

// Smoke and steam (src/ENGINE/Smoke.cpp): one camera-facing quad per puff,
// its billow texture turned by the puff's own rotation.

layout (location = 0) in vec2 corner;       // -0.5..0.5 (the shared particle quad)
layout (location = 1) in vec2 uv;
layout (location = 3) in vec4 iPosRadius;   // world position, radius now
layout (location = 4) in vec4 iParams;      // rotation, opacity now, steam, seed
layout (location = 5) in vec4 iColor;       // albedo (authored sRGB), glow

#include "camera.glsl"
#include "smoke_draw.glsl"

out vec2 vCorner;        // -1..1 across the puff, not turned (the frame the light is worked in)
out vec2 vTex;           // into the puff's billow, turned
out vec3 vWorld;
flat out vec3 vCenter;
flat out float vRadius;
flat out vec4 vParams;
flat out vec4 vColor;

void main()
{
	vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 up    = vec3(view[0][1], view[1][1], view[2][1]);
	vec2 c = corner * 2.0;
	float r = iPosRadius.w;
	vec3 world = iPosRadius.xyz + (right * c.x + up * c.y) * r;
	gl_Position = projection * view * vec4(world, 1.0);

	// One of four billows (a 2x2 atlas), turned. Turning keeps the length of
	// c, and only the unit circle is ever sampled, so it never strays into the
	// next cell.
	float a = iParams.x;
	vec2 t = vec2(c.x * cos(a) - c.y * sin(a), c.x * sin(a) + c.y * cos(a));
	int v = int(floor(iParams.w * 3.999));
	vec2 cell = vec2(float(v % 2), float(v / 2)) * 0.5;
	vTex = cell + (t * 0.5 + 0.5) * 0.5;

	vCorner = c;
	vWorld = world;
	vCenter = iPosRadius.xyz;
	vRadius = r;
	vParams = iParams;
	vColor = iColor;
}
