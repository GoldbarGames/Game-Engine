#ifndef KINJO_IBL_SAMPLING_GLSL
#define KINJO_IBL_SAMPLING_GLSL
// Low-discrepancy sampling for the image-based lighting precompute passes
// (render/Environment.cpp): Hammersley points and GGX importance sampling, as
// in Karis, "Real Shading in Unreal Engine 4" (SIGGRAPH 2013).
const float IBL_PI = 3.14159265358979;

float RadicalInverseVdC(uint bits)
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10;   // / 0x100000000
}

vec2 Hammersley(int i, int count)
{
	return vec2(float(i) / float(count), RadicalInverseVdC(uint(i)));
}

// Tangent frame around N.
void TangentFrame(vec3 N, out vec3 T, out vec3 B)
{
	vec3 up = (abs(N.z) < 0.999) ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
	T = normalize(cross(up, N));
	B = cross(N, T);
}

// A GGX-distributed half vector around N for roughness `rough` (alpha = rough^2).
vec3 ImportanceSampleGGX(vec2 xi, vec3 N, float rough)
{
	float a = rough * rough;
	float phi = 2.0 * IBL_PI * xi.x;
	float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	vec3 T, B;
	TangentFrame(N, T, B);
	return normalize(T * (cos(phi) * sinTheta) + B * (sin(phi) * sinTheta) + N * cosTheta);
}

float DistributionGGXIbl(float NdotH, float rough)
{
	float a = rough * rough;
	float a2 = a * a;
	float d = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(IBL_PI * d * d, 1e-6);
}
#endif
