#version 330

// The linear workflow's Resolve pass: the world's linear HDR colour, with
// bloom blended in, exposed and tonemapped, encoded to sRGB into the main
// framebuffer (which the composite, the toon outline and screenshots then use
// unchanged), then colour-graded through a lookup table when one is set
// (render/ColorGrading.h). A game can override this file for its own grading.

#include "draw.glsl"
#include "target.glsl"    // LinearToSrgb (and marks this shader linear-aware)
#include "tonemap.glsl"

struct DrawData
{
	float exposure;        // multiplier on the linear light (renderer.dat / .scene / scene3d exposure)
	int   tonemap;         // operator (tonemap.glsl)
	float bloomStrength;   // 0 = no bloom this frame
	float bloomLevels;     // the top bloom level holds this many levels' sum
	float gradeSize;       // the LUT's N (0 = none)
	float gradeStrength;
	float prevGradeSize;   // the look fading out during a cross-fade (0 = the ungraded image)
	float prevGradeStrength;
	float gradeBlend;      // from the previous look toward the current, 0..1
};
PER_DRAW(DrawData);

uniform sampler2D hdrColor;
uniform sampler2D bloomTexture;   // top (half-res) level of the bloom chain, linear filtered
uniform sampler2D gradeLut;       // strip LUT: N slices (blue), red across, green down
uniform sampler2D prevGradeLut;
out vec4 color;

// A display colour through a strip LUT: hardware bilinear in red and green,
// blended between the two nearest blue slices.
vec3 GradeLookup(sampler2D lut, float n, vec3 c)
{
	c = clamp(c, 0.0, 1.0);
	float slice = c.b * (n - 1.0);
	float s0 = floor(slice);
	float s1 = min(s0 + 1.0, n - 1.0);
	vec2 inSlice = vec2((c.r * (n - 1.0) + 0.5) / (n * n), (c.g * (n - 1.0) + 0.5) / n);
	vec3 a = textureLod(lut, inSlice + vec2(s0 / n, 0.0), 0.0).rgb;
	vec3 b = textureLod(lut, inSlice + vec2(s1 / n, 0.0), 0.0).rgb;
	return mix(a, b, slice - s0);
}

vec3 Grade(vec3 display)
{
	vec3 now = (draw.gradeSize > 0.0)
		? mix(display, GradeLookup(gradeLut, draw.gradeSize, display), draw.gradeStrength) : display;
	if (draw.gradeBlend >= 1.0)
		return now;
	vec3 before = (draw.prevGradeSize > 0.0)
		? mix(display, GradeLookup(prevGradeLut, draw.prevGradeSize, display), draw.prevGradeStrength) : display;
	return mix(before, now, draw.gradeBlend);
}

void main()
{
	vec4 h = texelFetch(hdrColor, ivec2(gl_FragCoord.xy), 0);
	vec3 c = max(h.rgb, vec3(0.0));

	// Bloom as a blend, not an add: the image is partly replaced by its own
	// blur, so flat areas keep their brightness and only light that is bright
	// against its surroundings visibly spreads.
	if (draw.bloomStrength > 0.0)
	{
		vec2 uv = gl_FragCoord.xy / vec2(textureSize(hdrColor, 0));
		vec3 bloom = texture(bloomTexture, uv).rgb / draw.bloomLevels;
		c = mix(c, bloom, draw.bloomStrength);
	}

	c = Tonemap(c * draw.exposure, draw.tonemap);
	vec3 display = LinearToSrgb(c);
	if (draw.gradeSize > 0.0 || draw.prevGradeSize > 0.0)
		display = Grade(display);
	color = vec4(display, clamp(h.a, 0.0, 1.0));
}
