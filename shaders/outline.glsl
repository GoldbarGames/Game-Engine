#ifndef KINJO_OUTLINE_GLSL
#define KINJO_OUTLINE_GLSL
// Settings for the toon outline composite (scene3d_edge.frag): the engine's
// "Outline" uniform block (std140, binding point 5 - see
// src/ENGINE/UniformBlocks.h), set by Game when the cel outline is on. The C++
// mirror is OutlineBlockData in Game.cpp (offsets checked at startup).
layout(std140) uniform Outline
{
	vec2  texelSize;       //  0  1 / screen size
	float nearPlane;       //  8  the camera's actual planes, for linearising depth
	float farPlane;        // 12
	vec3  outlineColor;    // 16
	float edgeThreshold;   // 28  world-unit sensitivity (smaller = more edges)
	float thickness;       // 32  outline width in pixels
};                         // 48 bytes
#endif
