#version 330

// Image-based lighting, step 3 (render/Environment.cpp): specular reflections,
// prefiltered per roughness - one mip level of the 256x128 specular panorama
// per roughness step (level / maxLevel). GGX importance sampling with the
// "filtered importance sampling" trick: each sample reads the environment
// base at a mip matching the solid angle it stands for, so few samples give a
// smooth result (Karis 2013; GPU Gems 3 ch. 20).

#include "draw.glsl"
#include "target.glsl"   // linear-aware
#include "panorama.glsl"
#include "ibl_sampling.glsl"

struct DrawData
{
	vec2  dstSize;     // this level's size
	float roughness;   // 0 = mirror
	float srcWidth;    // environment base width (texel solid angle)
};
PER_DRAW(DrawData);

uniform sampler2D envBase;
out vec4 color;

void main()
{
	vec2 uv = gl_FragCoord.xy / draw.dstSize;
	if (draw.roughness <= 0.0)
	{
		// Mirror level: the base itself, at this level's resolution.
		float lod = log2(draw.srcWidth / draw.dstSize.x);
		color = vec4(textureLod(envBase, uv, lod).rgb, 1.0);
		return;
	}

	vec3 N = PanoramaDir(uv);   // reflection direction; view = normal (isotropic approximation)
	const int COUNT = 128;
	float texelSolidAngle = 4.0 * IBL_PI / (draw.srcWidth * draw.srcWidth * 0.5);
	vec3 sum = vec3(0.0);
	float weight = 0.0;
	for (int i = 0; i < COUNT; i++)
	{
		vec3 H = ImportanceSampleGGX(Hammersley(i, COUNT), N, draw.roughness);
		vec3 L = normalize(2.0 * dot(N, H) * H - N);
		float NdotL = dot(N, L);
		if (NdotL <= 0.0)
			continue;
		float pdf = DistributionGGXIbl(max(dot(N, H), 0.0), draw.roughness) * 0.25 + 1e-4;   // N = V
		float sampleSolidAngle = 1.0 / (float(COUNT) * pdf + 1e-4);
		float lod = max(0.5 * log2(sampleSolidAngle / texelSolidAngle) + 1.0, 0.0);
		sum += textureLod(envBase, PanoramaUV(L), lod).rgb * NdotL;
		weight += NdotL;
	}
	color = vec4(sum / max(weight, 1e-4), 1.0);
}
