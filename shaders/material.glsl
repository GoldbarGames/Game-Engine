#ifndef KINJO_MATERIAL_GLSL
#define KINJO_MATERIAL_GLSL
// Surface material: the engine's "Material" uniform block (std140, binding
// point 2 - see src/ENGINE/UniformBlocks.h). Filled by Scene3D::ApplyMaterial;
// buffers are cached by content, so each distinct material (and each lake's
// water tuning) uploads once.
//
// Member names match the loose uniforms these replaced. The C++ mirror is
// MaterialBlockData in Scene3D.cpp (checked against these offsets at startup).

layout(std140) uniform Material
{
	vec3  matTint;            //  0
	float matFresnel;         // 12
	vec3  matEmissive;        // 16
	float matNormalStrength;  // 28
	vec2  matUVTile;          // 32
	float matSpecular;        // 40  Phong
	float matShininess;       // 44
	float matMetallic;        // 48  PBR
	float matRoughness;       // 52
	float matOpacity;         // 56
	int   matHasNormal;       // 60
	int   matNormalMode;      // 64  0 = screen-space TBN, 1 = vertex tangents
	int   matLighting;        // 68  0 = Blinn-Phong, 1 = PBR, 2 = water
	// Water tuning (matLighting == 2): vertex swells + fragment ripples.
	float uWaterAmp;          // 72  total wave height (world units)
	float uWaterWaveScale;    // 76  spatial frequency multiplier
	float uWaterShoreFade;    // 80  0 = waves reach the edge; 1 = calm shore
	float uWaterChoppy;       // 84  surface-ripple strength (1.0 = default)
};                            // 96 bytes (rounded to vec4)
#endif
