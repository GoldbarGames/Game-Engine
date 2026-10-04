#version 330

// Volumetric fog and light shafts, step 1 (Phase 1.5 item 9;
// src/ENGINE/render/VolumetricFog.h), at half resolution. Each pixel marches
// from the camera to the surface it sees (or the fog's reach) through a height
// fog, gathering at every step the light scattered towards the camera:
//   - the sun, through the cascaded shadow maps: god rays;
//   - every point and spot light of the step's cluster, through its cube
//     shadow: shafts from lamps;
//   - the sky's ambient light, and storm lightning;
// with a Henyey-Greenstein phase (forward scattering: shafts glow when you
// look towards the light) and energy-conserving integration per step
// (Hillaire 2015). Out: rgb = light scattered in, a = transmittance.
// Step offsets change per pixel and frame; temporal anti-aliasing integrates
// them into a smooth result.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"
#include "scene.glsl"
#include "camera.glsl"
#include "target.glsl"
#include "environment.glsl"
#include "cascades.glsl"
#include "lights.glsl"
#include "point_shadows.glsl"

struct DrawData
{
	mat4 invViewProj;   // the camera the world was drawn with
	vec4 medium;        // density (per world unit), height falloff, base height (world -Y up), anisotropy g
	vec4 tint;          // rgb scattering colour, noise amount (0 = smooth fog)
	vec4 params;        // reach (world units), steps, frame index, time (s)
	vec2 halfSize;      // this pass's size
	vec2 fullSize;      // the world target's
};
PER_DRAW(DrawData);

uniform sampler2D sceneDepth;   // the world's depth (full resolution)

out vec4 result;

const float PI = 3.14159265;

float Attenuate(float dist, float range)
{
	float a = clamp(1.0 - dist / range, 0.0, 1.0);
	return a * a;
}

// Henyey-Greenstein, relative to isotropic scattering (1 = the same in every
// direction), so the fog's brightness doesn't depend on g's normalisation.
float Phase(float cosTheta, float g)
{
	float g2 = g * g;
	return (1.0 - g2) / pow(max(1.0 + g2 - 2.0 * g * cosTheta, 1.0e-4), 1.5);
}

float Hash(vec3 p)
{
	p = fract(p * 0.3183099 + 0.1);
	p *= 17.0;
	return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

// Smooth value noise, 0..1.
float Noise(vec3 p)
{
	vec3 i = floor(p);
	vec3 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);
	return mix(mix(mix(Hash(i + vec3(0, 0, 0)), Hash(i + vec3(1, 0, 0)), f.x),
	               mix(Hash(i + vec3(0, 1, 0)), Hash(i + vec3(1, 1, 0)), f.x), f.y),
	           mix(mix(Hash(i + vec3(0, 0, 1)), Hash(i + vec3(1, 0, 1)), f.x),
	               mix(Hash(i + vec3(0, 1, 1)), Hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float Density(vec3 p)
{
	float height = -p.y - draw.medium.z;   // above the base (world up is -Y)
	float d = draw.medium.x * exp(-draw.medium.y * max(height, 0.0));
	if (draw.tint.w > 0.0)
	{
		// Slowly drifting patches (wind along +x), two octaves.
		vec3 q = p * 0.0025 + vec3(draw.params.w * 0.05, 0.0, draw.params.w * 0.02);
		float n = Noise(q) * 0.65 + Noise(q * 2.7) * 0.35;
		d *= mix(1.0, n * 2.0, draw.tint.w);
	}
	return d;
}

void main()
{
	ivec2 fullPix = min(ivec2(gl_FragCoord.xy) * 2, ivec2(draw.fullSize) - 1);
	vec2 pixelCentre = vec2(fullPix) + 0.5;
	float depth = texelFetch(sceneDepth, fullPix, 0).r;
	vec4 w = draw.invViewProj * vec4(pixelCentre / draw.fullSize * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
	vec3 surface = w.xyz / w.w;

	vec3 ray = surface - viewPos;
	float dist = length(ray);
	vec3 dir = ray / max(dist, 1.0e-4);
	float reach = min(dist, draw.params.x);
	int steps = int(draw.params.y);
	float stepLength = reach / float(steps);

	// A different offset per pixel and frame (interleaved gradient noise).
	vec2 noisePos = gl_FragCoord.xy + 5.588238 * mod(draw.params.z, 64.0);
	float jitter = fract(52.9829189 * fract(dot(noisePos, vec2(0.06711056, 0.00583715))));

	float g = draw.medium.w;
	vec3 sun = dirLightColor * dirLightDiffuse;
	float sunPhase = Phase(dot(-normalize(dirLightDir), dir), g);
	vec3 ambient = ((iblOn != 0) ? EnvDiffuseFlat() : ambientColor) + lightningColor * lightningFlash;

	float transmittance = 1.0;
	vec3 scattered = vec3(0.0);
	for (int i = 0; i < 64; i++)
	{
		if (i >= steps)
			break;
		vec3 p = viewPos + dir * ((float(i) + jitter) * stepLength);
		float density = Density(p);
		if (density <= 0.0)
			continue;

		vec3 light = ambient;
		if (dirLightDiffuse > 0.0)
			light += sun * sunPhase * SunShadowVolume(p);

		int first, count;
		LightRangeAt(pixelCentre, p, first, count);
		for (int k = first; k < first + count; k++)
		{
			SceneLight l = GetLight(k);
			vec3 toLight = l.pos - p;
			float d = length(toLight);
			if (d >= l.range || d < 1.0e-3)
				continue;
			vec3 L = toLight / d;
			vec3 radiance = l.color * l.intensity * Attenuate(d, l.range);
			if (l.spot != 0)
				radiance *= smoothstep(l.cosOuter, l.cosInner, dot(-L, normalize(l.dir)));
			else if (l.shadow >= 0)
				radiance *= PointShadowSlot(l.shadow, p);
			light += radiance * Phase(dot(L, dir), g);
		}

		// Energy-conserving step: what this slab scatters, dimmed by what lies
		// between it and the camera.
		float stepTransmittance = exp(-density * stepLength);
		scattered += transmittance * draw.tint.rgb * light * (1.0 - stepTransmittance);
		transmittance *= stepTransmittance;
		if (transmittance < 0.003)
			break;
	}
	result = vec4(scattered, transmittance);
}
