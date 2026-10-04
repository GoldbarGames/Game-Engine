#ifndef KINJO_TARGET_GLSL
#define KINJO_TARGET_GLSL
// The render target this draw writes, and colour-space helpers (the linear
// workflow, see src/ENGINE/render/ColorPipeline.h).
//
// In a project with `linearLighting 1`, the world renders into a linear HDR
// target and kinjoTargetLinear is 1 there; everywhere else (and in every
// project without it) it is 0.
//
// Including this file marks the shader as LINEAR-AWARE: it decides for itself
// what it writes into a linear target. Every other fragment shader is treated
// as gamma-space art and the engine converts its output on the way into a
// linear target, so 2D content keeps its exact look.
#define KINJO_LINEAR_OUTPUT

layout(std140) uniform KinjoTarget
{
	int kinjoTargetLinear;
};

// The exact sRGB transfer curves (not a 2.2 power).
vec3 SrgbToLinear(vec3 c)
{
	return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

vec3 LinearToSrgb(vec3 c)
{
	return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

// An authored (sRGB) colour constant, in the space this draw writes.
vec3 TargetColor(vec3 srgb)
{
	return (kinjoTargetLinear != 0) ? SrgbToLinear(srgb) : srgb;
}
#endif
