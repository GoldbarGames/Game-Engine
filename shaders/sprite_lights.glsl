#ifndef KINJO_SPRITE_LIGHTS_GLSL
#define KINJO_SPRITE_LIGHTS_GLSL
// The renderer's point lights for lit sprites (shader.frag): the engine's
// "SpriteLights" uniform block (std140, binding point 3 - see
// src/ENGINE/UniformBlocks.h), filled by Renderer::UseLight. The C++ mirror is
// SpriteLightsBlockData in Renderer.cpp (offsets checked at startup).

const int MAX_POINT_LIGHTS = 8;

// Base light (matches the engine's Light class)
struct Light
{
	vec3  color;              // 0
	float ambientIntensity;   // 12
	float diffuseIntensity;   // 16
};                            // std140 size 32

// Point light with attenuation (matches the engine's PointLight class)
struct PointLight
{
	Light base;               // 0
	vec3  position;           // 32
	float constant;           // 44
	float linear;             // 48
	float exponent;           // 52
};                            // std140 size 64

layout(std140) uniform SpriteLights
{
	int pointLightCount;                       // 0
	PointLight pointLights[MAX_POINT_LIGHTS];  // 16 (stride 64)
};                                             // 528 bytes
#endif
