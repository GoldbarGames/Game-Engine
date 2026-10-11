#version 330 core

// Flames and sparks (src/ENGINE/Smoke.cpp, EmitFlame; docs/VISUAL_EFFECTS.md).
//
// A flame is a tongue: wide at its root, tapering to a tip that wavers, with
// noise rising up through it so it tears into separate wisps toward the top.
// Its colour is its temperature - a white-yellow heart at the root, orange,
// then a dull red at the ragged edges and tips - and it is light, not a thing
// that is lit: it is added to what is behind it (written with no coverage, so
// the premultiplied blend adds it). A spark is a thin hard streak, hottest at
// its head.

in vec2 vUV;
in vec3 vWorld;
flat in vec4 vParams;    // age / life, brightness, temperature, seed
flat in float vSpark;
flat in float vWidth;

#include "camera.glsl"
#include "scene.glsl"
#include "target.glsl"
#include "distance_fog.glsl"
#include "smoke_draw.glsl"

uniform sampler2D sceneDepth;   // unit 1: the world's depth, when draw.uDepth.z = 1

layout(location = 0) out vec4 color;

float ViewDepth(float d)
{
	float n = draw.uDepth.x, f = draw.uDepth.y;
	return (2.0 * n * f) / (f + n - (d * 2.0 - 1.0) * (f - n));
}

float Hash(vec2 p)
{
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float Noise(vec2 p)
{
	vec2 i = floor(p), f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	float a = Hash(i), b = Hash(i + vec2(1.0, 0.0));
	float c = Hash(i + vec2(0.0, 1.0)), d = Hash(i + vec2(1.0, 1.0));
	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float Fbm(vec2 p)
{
	return 0.55 * Noise(p) + 0.3 * Noise(p * 2.1 + 3.7) + 0.15 * Noise(p * 4.3 - 1.9);
}

// White-yellow at 1, through yellow and orange, to a dull red at 0.
vec3 Heat(float t)
{
	vec3 c = mix(vec3(0.45, 0.04, 0.0), vec3(0.95, 0.25, 0.03), smoothstep(0.0, 0.35, t));
	c = mix(c, vec3(1.0, 0.55, 0.12), smoothstep(0.3, 0.6, t));
	c = mix(c, vec3(1.0, 0.82, 0.42), smoothstep(0.55, 0.85, t));
	return mix(c, vec3(1.0, 0.96, 0.85), smoothstep(0.85, 1.0, t));
}

void main()
{
	float life = vParams.x;
	float clock = draw.uFade.z;
	float seed = vParams.w * 61.0;
	float x = vUV.x, y = vUV.y;
	float glow, heat;

	if (vSpark > 0.5)
	{
		// A streak: hard across, hottest at its head, its tail fading.
		float across = 1.0 - abs(x);
		glow = across * across * smoothstep(0.0, 0.7, y) * (1.0 - smoothstep(0.92, 1.0, y));
		heat = 0.55 + 0.45 * y;
		glow *= 1.0 - smoothstep(0.5, 1.0, life);       // cooling
	}
	else
	{
		// The tongue wavers: its middle line wanders, more toward the tip.
		float wander = (Fbm(vec2(y * 2.2 - clock * 2.6, seed)) - 0.5) * 1.1 * y;
		float xs = x * 1.6 - wander;                     // the quad is 1.6 times its width
		float halfW = pow(max(1.0 - y, 0.0), 0.65) * (0.8 + 0.2 * sin(y * 11.0 - clock * 9.0 + seed));
		float body = 1.0 - smoothstep(halfW * 0.35, max(halfW, 0.001), abs(xs));
		// Noise rising through it tears it into wisps, the more so higher up.
		float rising = Fbm(vec2(xs * 2.6 + seed, y * 3.4 - clock * 5.5));
		float wisps = smoothstep(0.2 + 0.5 * y, 0.55 + 0.4 * y, rising + 0.45 * (1.0 - y));
		glow = body * wisps * smoothstep(0.0, 0.1, y);
		// Hottest at the heart of the root.
		heat = (1.0 - y) * (1.0 - abs(xs) / max(halfW, 0.001)) * 1.1 + 0.1 * (1.0 - y);
		// Born quickly, gone at the end.
		glow *= smoothstep(0.0, 0.12, life) * (1.0 - smoothstep(0.55, 1.0, life));
	}
	if (glow < 0.004)
		discard;

	// Behind the world, none of it; just in front of it, thinning to nothing.
	if (draw.uDepth.z > 0.5)
	{
		float scene = ViewDepth(texelFetch(sceneDepth, ivec2(gl_FragCoord.xy), 0).r);
		float eye = ViewDepth(gl_FragCoord.z);
		glow *= clamp((scene - eye) / max(draw.uDepth.w * vWidth, 0.03), 0.0, 1.0);
		if (glow < 0.004)
			discard;
	}

	float t = clamp(vParams.z * heat, 0.0, 1.0);
	vec3 c = TargetColor(Heat(t)) * vParams.y * glow;
	// Light added to the picture fades into the fog rather than taking its colour.
	c *= 1.0 - DistanceFogAmount(vWorld);
	color = vec4(c, 0.0);
}
