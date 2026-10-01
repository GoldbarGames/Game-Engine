#ifndef KINJO_WEATHER_DRAW_GLSL
#define KINJO_WEATHER_DRAW_GLSL
// Per-draw values for weather / fountain particles (one instanced draw per
// pass), shared by weather.vert and weather.frag. 60 bytes.
#include "draw.glsl"

struct DrawData
{
	vec4  uColor;     // particle tint (rgb) and opacity
	vec3  uCamPos;    // camera world position (rain streak orientation)
	float uTime;      // seconds, for snow sway
	vec3  uFallDir;   // normalized fall direction (down = +Y)
	float uSize;      // snow flake size / rain streak width
	float uLength;    // rain streak length (snow uses uSize)
	float uSway;      // snow horizontal sway amplitude (world units)
	int   uMode;      // 0 = snow (dot), 1 = rain streak, 2 = fountain streak (along iVel)
};
PER_DRAW(DrawData);
#endif
