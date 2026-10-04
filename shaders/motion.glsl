#ifndef KINJO_MOTION_GLSL
#define KINJO_MOTION_GLSL
// Screen-space motion vectors for temporal anti-aliasing (Phase 1.5 item 7;
// src/ENGINE/render/TemporalAA.h): where each surface point was on screen last
// frame, so TAA can find its history even when the object itself moves - a
// walking character, a game-moved prop - not just when the camera does.
//
// The world target's colour attachment 2 receives them, only while the engine
// enables it for a draw (Scene3DInternal::WorldDrawBuffers); a shader that
// includes this file declares `layout(location = 2) out vec4` for it.
layout(std140) uniform Motion
{
	mat4 motionViewProj;       // this frame's camera, without the TAA jitter
	mat4 motionPrevViewProj;   // last frame's
	int  motionOn;             // 0: nothing reads them this frame
};

// rg = this frame's uv minus last frame's (TAA's convention), b = 1 (an object
// wrote it; elsewhere TAA reprojects through the depth buffer), a = coverage:
// alpha blending is on for world draws, so 0 keeps what lies behind (glass)
// and 1 replaces it.
vec4 MotionVector(vec3 worldPos, vec3 prevWorldPos, float coverage)
{
	if (motionOn == 0)
		return vec4(0.0);
	vec4 current = motionViewProj * vec4(worldPos, 1.0);
	vec4 previous = motionPrevViewProj * vec4(prevWorldPos, 1.0);
	if (current.w <= 0.0 || previous.w <= 0.0)
		return vec4(0.0);
	vec2 v = (current.xy / current.w - previous.xy / previous.w) * 0.5;
	return vec4(v, 1.0, coverage);
}
#endif
