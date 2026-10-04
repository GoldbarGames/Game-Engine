#ifndef KINJO_AO_GLSL
#define KINJO_AO_GLSL
// Screen-space ambient occlusion (Phase 1.5 item 6;
// src/ENGINE/render/AmbientOcclusion.h). Linear workflow only: aoOn is 0 in
// gamma projects, without a lit 3D scene, or with `ao 0`.
//
// It darkens AMBIENT light only (the sky's light or the flat ambient colour,
// and sky reflections), never direct light.
#include "camera.glsl"   // view: this fragment's depth, to match the prepass

layout(std140) uniform AmbientOcclusion
{
	int   aoOn;
	float aoStrength;   // 0..1 blend toward full occlusion (.scene / renderer.dat `ao`)
	int   aoDebug;      // 1 = show the occlusion instead of the lit colour (KINJO_AO_DEBUG)
	int   ssrOn;        // screen-space reflections this frame (render/Reflections.h)
};

uniform sampler2D aoMap;     // denoised visibility, 1 = unoccluded (unit 12)
uniform sampler2D aoDepth;   // the prepass's linear view depth (unit 13)
uniform sampler2D ssrMap;    // screen-space reflections: rgb light, a confidence (unit 15)

// Visibility of ambient light at this fragment: 1 = fully lit. A fragment the
// prepass didn't draw (glass, water, anything not a scene model) sits at a
// different depth than the prepass recorded there, and is left unoccluded.
float AmbientVisibility(vec3 worldPos)
{
	if (aoOn == 0)
		return 1.0;
	ivec2 p = ivec2(gl_FragCoord.xy);
	float prepassDepth = texelFetch(aoDepth, p, 0).r;
	float depth = -(view * vec4(worldPos, 1.0)).z;
	if (abs(prepassDepth - depth) > 0.01 * depth + 1.0)
		return 1.0;
	return mix(1.0, texelFetch(aoMap, p, 0).r, aoStrength);
}

// What this fragment's surface reflects from the screen: rgb = light, a =
// confidence (0 = nothing found; keep the sky reflection). Only for the
// surface the prepass recorded here, like the occlusion.
vec4 ScreenReflection(vec3 worldPos)
{
	if (ssrOn == 0)
		return vec4(0.0);
	ivec2 p = ivec2(gl_FragCoord.xy);
	float prepassDepth = texelFetch(aoDepth, p, 0).r;
	float depth = -(view * vec4(worldPos, 1.0)).z;
	if (abs(prepassDepth - depth) > 0.01 * depth + 1.0)
		return vec4(0.0);
	return texelFetch(ssrMap, p, 0);
}

// How much of a reflection survives the same occlusion (Lagarde & de Rousiers,
// "Moving Frostbite to PBR", 2014): rough and head-on surfaces follow the
// ambient occlusion; smooth surfaces at grazing angles keep more.
float SpecularVisibility(float NdotV, float visibility, float roughness)
{
	return clamp(pow(NdotV + visibility, exp2(-16.0 * roughness - 1.0)) - 1.0 + visibility, 0.0, 1.0);
}
#endif
