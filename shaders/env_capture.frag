#version 330

// Image-based lighting, step 1 (render/Environment.cpp): the scene's sky
// panorama - cross-fade and tint included, decoded to linear light - resampled
// into the 512x256 environment base. Same equirectangular layout as the
// panorama itself, so this is a straight box-filtered downsample, no warping.

#include "draw.glsl"
#include "target.glsl"   // SrgbToLinear (and linear-aware)

struct DrawData
{
	vec4  tint;      // the sky's tint, already linear
	vec2  dstSize;   // the environment base, in pixels
	float blend;     // cross-fade toward nextTexture (0..1)
};
PER_DRAW(DrawData);

uniform sampler2D skyTexture;
uniform sampler2D nextTexture;
out vec4 color;

// Average the block of source texels under this destination texel (up to 4x4),
// decoding each first so the average is of light, not of encoded values.
vec3 BoxSample(sampler2D tex)
{
	ivec2 size = textureSize(tex, 0);
	vec2 ratio = vec2(size) / draw.dstSize;
	ivec2 n = clamp(ivec2(ceil(ratio)), ivec2(1), ivec2(4));
	vec2 start = floor(gl_FragCoord.xy) * ratio;
	vec3 sum = vec3(0.0);
	for (int j = 0; j < 4; j++)
	{
		for (int i = 0; i < 4; i++)
		{
			if (i >= n.x || j >= n.y)
				continue;
			ivec2 p = ivec2(start + (vec2(i, j) + 0.5) * ratio / vec2(n));
			p = clamp(p, ivec2(0), size - 1);
			sum += SrgbToLinear(texelFetch(tex, p, 0).rgb);
		}
	}
	return sum / float(n.x * n.y);
}

void main()
{
	vec3 c = BoxSample(skyTexture);
	if (draw.blend > 0.0)
		c = mix(c, BoxSample(nextTexture), draw.blend);
	color = vec4(c * draw.tint.rgb, 1.0);
}
