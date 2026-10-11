#version 330

// Image-based lighting, step 1 (render/Environment.cpp): the scene's sky
// panorama - cross-fade and tint included, decoded to linear light - resampled
// into the 512x256 environment base. Same equirectangular layout as the
// panorama itself, so this is a straight box-filtered downsample, no warping.
// With a sun in the sky (src/ENGINE/SkyBodies.h), its glow and the dawn colours
// are added as the sky draws them, so the world is lit by the sky it shows.

#include "draw.glsl"
#include "target.glsl"   // SrgbToLinear (and linear-aware)
#include "panorama.glsl"
#include "sky_bodies.glsl"

struct DrawData
{
	vec4  tint;      // the sky's tint, already linear
	vec2  dstSize;   // the environment base, in pixels
	float blend;     // cross-fade toward nextTexture (0..1)
	float skyLight;  // 1: add the sun's light in the air (sky_bodies.glsl)
	float linearSky; // 1: the panorama is linear already (a sky the game painted, Scene3D::SetSkyImage)
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
			vec3 v = texelFetch(tex, p, 0).rgb;
			sum += (draw.linearSky > 0.5) ? v : SrgbToLinear(v);
		}
	}
	return sum / float(n.x * n.y);
}

void main()
{
	vec3 c = BoxSample(skyTexture);
	if (draw.blend > 0.0)
		c = mix(c, BoxSample(nextTexture), draw.blend);
	c *= draw.tint.rgb;
	if (draw.skyLight > 0.5)
	{
		// In the panorama's sRGB terms, as sky.frag adds it. The sky is smooth at
		// this size, so encoding the averaged texel is near enough.
		vec3 d = PanoramaDir((floor(gl_FragCoord.xy) + 0.5) / draw.dstSize);
		c = SrgbToLinear(min(SkySunAir(LinearToSrgb(c), d), vec3(1.0)));
	}
	color = vec4(c, 1.0);
}
