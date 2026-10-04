#version 330

// Image-based lighting, step 2 (render/Environment.cpp): diffuse irradiance.
// For each direction N, the cosine-weighted average of the sky over the
// hemisphere around N, so a surface's ambient is simply albedo * this. Stored
// as a small 64x32 panorama; the sky is sampled from a blurred mip of the
// environment base, so 256 samples are plenty.

#include "draw.glsl"
#include "target.glsl"   // linear-aware
#include "panorama.glsl"
#include "ibl_sampling.glsl"

struct DrawData
{
	vec2  dstSize;
	float srcLod;    // environment-base mip to sample
};
PER_DRAW(DrawData);

uniform sampler2D envBase;
out vec4 color;

void main()
{
	vec2 uv = gl_FragCoord.xy / draw.dstSize;
	vec3 N = PanoramaDir(uv);
	vec3 T, B;
	TangentFrame(N, T, B);

	const int COUNT = 256;
	vec3 sum = vec3(0.0);
	for (int i = 0; i < COUNT; i++)
	{
		// Cosine-weighted hemisphere: the cosine is in the distribution, so a
		// plain average of the samples is the irradiance / pi.
		vec2 xi = Hammersley(i, COUNT);
		float phi = 2.0 * IBL_PI * xi.x;
		float cosTheta = sqrt(1.0 - xi.y);
		float sinTheta = sqrt(xi.y);
		vec3 L = T * (cos(phi) * sinTheta) + B * (sin(phi) * sinTheta) + N * cosTheta;
		sum += textureLod(envBase, PanoramaUV(L), draw.srcLod).rgb;
	}
	color = vec4(sum / float(COUNT), 1.0);
}
