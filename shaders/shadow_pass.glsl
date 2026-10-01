#ifndef KINJO_SHADOW_PASS_GLSL
#define KINJO_SHADOW_PASS_GLSL
// One shadow-map render target: the sun's map or one point-light cube face.
// The engine's "ShadowPass" uniform block (std140, binding point 4 - see
// src/ENGINE/UniformBlocks.h), set by Scene3D's shadow passes. The C++ mirror
// is ShadowPassBlockData in Scene3D.cpp (offsets checked at startup).
layout(std140) uniform ShadowPass
{
	mat4  viewProj;      //  0  light projection * light view (or cube face)
	vec3  lightPos;      // 64  point light position (cube passes)
	float farPlane;      // 76  cube pass far plane (depth stored as dist / far)
	float alphaCutoff;   // 80  transparent sprite pixels cast no shadow
};                       // 96 bytes
#endif
