#ifndef KINJO_ENVIRONMENT_GLSL
#define KINJO_ENVIRONMENT_GLSL
// Image-based lighting from the scene's sky (Phase 1.5 item 4;
// src/ENGINE/render/Environment.h). Linear workflow only: iblOn is 0 in gamma
// projects, scenes without a sky, or with `ibl 0`.
#include "panorama.glsl"

layout(std140) uniform Environment
{
	int   iblOn;
	float iblDiffuse;    // multiplier on the sky's diffuse light (.scene / renderer.dat `ibl`)
	float iblSpecular;   // multiplier on its reflections
	float envMaxLod;     // specular panorama's last mip = roughness 1
};

uniform sampler2D envIrradiance;   // cosine-convolved sky panorama (unit 8)
uniform sampler2D envSpecular;     // GGX-prefiltered sky, roughness = lod / envMaxLod (unit 9)
uniform sampler2D brdfLUT;         // split-sum scale/bias: x = NdotV, y = roughness (unit 10)

// The sky's diffuse light arriving at a surface facing N (multiply by albedo).
vec3 EnvDiffuse(vec3 N)
{
	return textureLod(envIrradiance, PanoramaUV(N), 0.0).rgb * iblDiffuse;
}

// Direction-independent ambient from the sky (flat-shaded looks: toon,
// billboards): the six axis directions averaged.
vec3 EnvDiffuseFlat()
{
	vec3 s = textureLod(envIrradiance, PanoramaUV(vec3( 1.0, 0.0, 0.0)), 0.0).rgb
	       + textureLod(envIrradiance, PanoramaUV(vec3(-1.0, 0.0, 0.0)), 0.0).rgb
	       + textureLod(envIrradiance, PanoramaUV(vec3( 0.0, 1.0, 0.0)), 0.0).rgb
	       + textureLod(envIrradiance, PanoramaUV(vec3( 0.0,-1.0, 0.0)), 0.0).rgb
	       + textureLod(envIrradiance, PanoramaUV(vec3( 0.0, 0.0, 1.0)), 0.0).rgb
	       + textureLod(envIrradiance, PanoramaUV(vec3( 0.0, 0.0,-1.0)), 0.0).rgb;
	return s / 6.0 * iblDiffuse;
}

// The sky reflected along R from a surface of the given roughness.
vec3 EnvReflection(vec3 R, float rough)
{
	return textureLod(envSpecular, PanoramaUV(R), clamp(rough, 0.0, 1.0) * envMaxLod).rgb * iblSpecular;
}

vec2 EnvBrdf(float NdotV, float rough)
{
	return textureLod(brdfLUT, vec2(clamp(NdotV, 0.0, 1.0), clamp(rough, 0.0, 1.0)), 0.0).rg;
}
#endif
