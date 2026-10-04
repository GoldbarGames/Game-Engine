#version 330

// Bloom, downsample step (Phase 1.5 item 3; src/ENGINE/render/ColorPipeline.cpp).
// The 13-tap filter from Jimenez, "Next Generation Post Processing in Call of
// Duty: Advanced Warfare" (SIGGRAPH 2014): one level of the mip chain from the
// level above, without the aliasing a plain 2x2 box gives. The first step
// (karis = 1) takes a Karis average - each 2x2 group weighted by 1/(1+luma) -
// so a single very bright pixel (a water glint) can't flicker into a big blob.

#include "draw.glsl"
#include "target.glsl"   // linear-aware: works on linear HDR light

struct DrawData
{
	vec2 srcTexel;   // 1 / source size
	vec2 dstSize;    // this level's size in pixels
	int  karis;      // 1 on the first step
};
PER_DRAW(DrawData);

uniform sampler2D srcTexture;
out vec4 color;

vec3 Fetch(vec2 uv)
{
	vec3 c = texture(srcTexture, uv).rgb;
	// A NaN/inf in the scene would otherwise spread over the whole screen.
	if (any(isnan(c)) || any(isinf(c)))
		return vec3(0.0);
	return max(c, vec3(0.0));
}

float KarisWeight(vec3 c)
{
	return 1.0 / (1.0 + dot(c, vec3(0.2126, 0.7152, 0.0722)));
}

void main()
{
	vec2 uv = gl_FragCoord.xy / draw.dstSize;
	vec2 t = draw.srcTexel;

	vec3 a = Fetch(uv + t * vec2(-2.0,  2.0));
	vec3 b = Fetch(uv + t * vec2( 0.0,  2.0));
	vec3 c = Fetch(uv + t * vec2( 2.0,  2.0));
	vec3 d = Fetch(uv + t * vec2(-2.0,  0.0));
	vec3 e = Fetch(uv);
	vec3 f = Fetch(uv + t * vec2( 2.0,  0.0));
	vec3 g = Fetch(uv + t * vec2(-2.0, -2.0));
	vec3 h = Fetch(uv + t * vec2( 0.0, -2.0));
	vec3 i = Fetch(uv + t * vec2( 2.0, -2.0));
	vec3 j = Fetch(uv + t * vec2(-1.0,  1.0));
	vec3 k = Fetch(uv + t * vec2( 1.0,  1.0));
	vec3 l = Fetch(uv + t * vec2(-1.0, -1.0));
	vec3 m = Fetch(uv + t * vec2( 1.0, -1.0));

	vec3 result;
	if (draw.karis != 0)
	{
		// Five overlapping 2x2 groups: four corners (0.125 each), centre (0.5).
		vec3 g0 = (a + b + d + e) * 0.25;
		vec3 g1 = (b + c + e + f) * 0.25;
		vec3 g2 = (d + e + g + h) * 0.25;
		vec3 g3 = (e + f + h + i) * 0.25;
		vec3 g4 = (j + k + l + m) * 0.25;
		float w0 = KarisWeight(g0) * 0.125;
		float w1 = KarisWeight(g1) * 0.125;
		float w2 = KarisWeight(g2) * 0.125;
		float w3 = KarisWeight(g3) * 0.125;
		float w4 = KarisWeight(g4) * 0.5;
		result = (g0 * w0 + g1 * w1 + g2 * w2 + g3 * w3 + g4 * w4) / (w0 + w1 + w2 + w3 + w4);
	}
	else
	{
		result = e * 0.125
			+ (a + c + g + i) * 0.03125
			+ (b + d + f + h) * 0.0625
			+ (j + k + l + m) * 0.125;
	}
	color = vec4(result, 1.0);
}
