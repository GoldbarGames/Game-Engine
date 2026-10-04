#version 330

// Image-based lighting, step 0 (render/Environment.cpp; computed once): the
// split-sum BRDF lookup table. For (NdotV, roughness) it stores the scale and
// bias applied to F0 (Karis 2013), so a reflection is
//     prefiltered(R, roughness) * (F0 * lut.x + lut.y).

#include "draw.glsl"
#include "target.glsl"   // linear-aware
#include "ibl_sampling.glsl"

struct DrawData
{
	vec2 dstSize;
};
PER_DRAW(DrawData);

out vec4 color;

float GeometrySchlickIbl(float NdotX, float rough)
{
	float k = (rough * rough) * 0.5;   // the IBL remapping of k
	return NdotX / (NdotX * (1.0 - k) + k);
}

vec2 IntegrateBrdf(float NdotV, float rough)
{
	vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);
	vec3 N = vec3(0.0, 0.0, 1.0);
	float a = 0.0;
	float b = 0.0;
	const int COUNT = 256;
	for (int i = 0; i < COUNT; i++)
	{
		vec3 H = ImportanceSampleGGX(Hammersley(i, COUNT), N, rough);
		vec3 L = normalize(2.0 * dot(V, H) * H - V);
		float NdotL = max(L.z, 0.0);
		float NdotH = max(H.z, 0.0);
		float VdotH = max(dot(V, H), 0.0);
		if (NdotL > 0.0)
		{
			float G = GeometrySchlickIbl(NdotV, rough) * GeometrySchlickIbl(NdotL, rough);
			float gVis = G * VdotH / max(NdotH * NdotV, 1e-5);
			float fc = pow(1.0 - VdotH, 5.0);
			a += (1.0 - fc) * gVis;
			b += fc * gVis;
		}
	}
	return vec2(a, b) / float(COUNT);
}

void main()
{
	vec2 uv = gl_FragCoord.xy / draw.dstSize;
	color = vec4(IntegrateBrdf(max(uv.x, 1e-3), uv.y), 0.0, 1.0);
}
