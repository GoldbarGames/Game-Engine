#version 330

// A sky the game painted itself, in linear HDR (Scene3D::SetSkyImage,
// src/ENGINE/SkyImage.cpp): drawn as it is, with no sRGB decode and no clamp, so
// a sun painted far brighter than 1 blooms. Drawn on the sky sphere by
// sky_hdr.vert. The sun's disc (Scene3D::SetSkyImageSun) is worked out here per
// pixel: painted into the image it would be a texel or two of blur.

in vec3 Dir;

layout(location = 0) out vec4 color;

uniform sampler2D skyTexture;   // unit 0: the painted panorama (RGBA16F, linear)

#include "draw.glsl"

struct DrawData   // as sky_hdr.vert's
{
	vec4 p0;     // a multiplier rgb (1), -
	vec4 p1;     // the sky's radius, -, -, -
	vec4 sun0;   // toward the sun xyz, sin(its angular radius)
	vec4 sun1;   // the disc's radiance rgb (0: none), flatten
	vec4 sun2;   // its lower edge's tint rgb, -
	vec4 sun3;   // its upper edge's tint rgb, -
};
PER_DRAW(DrawData);

#include "target.glsl"
#include "panorama.glsl"

void main()
{
	vec3 d = normalize(Dir);
	// Across the panorama's seam u jumps by 1: taken out of the derivatives, or
	// the seam draws as a line of the smallest mip.
	vec2 uv = PanoramaUV(d);
	vec2 dx = dFdx(uv), dy = dFdy(uv);
	dx.x -= floor(dx.x + 0.5);
	dy.x -= floor(dy.x + 0.5);
	vec3 sky = textureGrad(skyTexture, uv, dx, dy).rgb * draw.p0.rgb;

	// The sun's disc, soft over half its radius. A low one is squashed by
	// refraction (measured across, up the sky, divided by `flatten`) and redder
	// at its lower edge, which crosses more air.
	if (draw.sun1.r + draw.sun1.g + draw.sun1.b > 0.0)
	{
		vec3 s = normalize(draw.sun0.xyz);
		vec3 zenith = vec3(0.0, -1.0, 0.0);
		vec3 right = cross(s, zenith);
		right = (dot(right, right) > 1e-8) ? normalize(right) : vec3(1.0, 0.0, 0.0);
		vec3 up = cross(right, s);
		up *= sign(dot(up, zenith) + 1e-6);   // toward the zenith, whichever way the cross products turn
		float r = draw.sun0.w;
		float x = dot(d, right), y = dot(d, up) / draw.sun1.w;
		float disc = (1.0 - smoothstep(r, r * 1.5, length(vec2(x, y)))) * step(0.0, dot(d, s));
		vec3 tint = mix(draw.sun2.rgb, draw.sun3.rgb, clamp(y / r * 0.5 + 0.5, 0.0, 1.0));
		sky += draw.sun1.rgb * tint * disc;
	}

	// Linear light: as it is into a linear target, encoded into a gamma one
	color = vec4((kinjoTargetLinear != 0) ? sky : LinearToSrgb(clamp(sky, 0.0, 1.0)), 1.0);
}
