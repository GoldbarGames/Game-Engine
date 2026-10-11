#version 330

// THE SKY: its panorama, with the sun and the moon where the game has put them
// (src/ENGINE/SkyBodies.h). The panorama is painted without them. Over it, in
// its own sRGB terms: the sun's glow and the dawn colours (sky_bodies.glsl,
// which image-based lighting shares), the moon, and the sun's glare and disc.

in vec3 Dir;

layout(location = 0) out vec4 color;

uniform sampler2D skyTexture;    // unit 0: the panorama
uniform sampler2D nextTexture;   // unit 1: the one it is fading into
uniform sampler2D moonFace;      // unit 2: the moon's face

#include "draw.glsl"

struct DrawData
{
	vec4 p0;   // the panorama's tint rgb, cross-fade toward the next (0..1)
	vec4 p1;   // the sky's radius, has a moon face (0/1), the face's width in texels, -
};
PER_DRAW(DrawData);

// Linear-aware (render/ColorPipeline.h): worked in sRGB, as the panoramas are
// painted, and converted for the target at the end.
#include "target.glsl"
#include "panorama.glsl"
#include "sky_bodies.glsl"

// The moon: the glow round it, then its disc - its face lit on the side toward
// the sun, so its phase is where the two are. `behind`: the sky with no stars
// in it (a blurred mip), which is what its dark part shows.
vec3 SkyMoon(vec3 sky, vec3 behind, vec3 d, float pix)
{
	if (skyMoon.w <= 0.0)
		return sky;
	vec3 m = normalize(skyMoon.xyz);
	vec3 s = normalize(skySun.xyz);
	float full = clamp((1.0 - dot(m, s)) * 0.5, 0.0, 1.0);   // 0 new .. 1 full
	float above = smoothstep(-pix, pix, -d.y);                // the ground hides what is below
	// Its glow is in the air, in front of it as well as round it.
	float dm = dot(d, m);
	vec3 glow = skyMoonGlow.w * full * full * (0.20 * exp((dm - 1.0) * 2500.0) + 0.07 * exp((dm - 1.0) * 180.0))
		* skyMoonGlow.rgb;
	sky += glow;
	behind += glow;

	float R = skyMoonColour.w;
	float ang = 2.0 * asin(clamp(length(d - m) * 0.5, 0.0, 1.0));
	if (ang > R + 3.0 * pix)
		return min(sky, vec3(1.0));
	// Across its face: right and up as seen looking at it, its north up.
	vec3 north = skyMoonNorth.xyz - m * dot(skyMoonNorth.xyz, m);
	north = (length(north) > 1e-4) ? normalize(north) : normalize(cross(m, vec3(1.0, 0.0, 0.0)));
	vec3 right = cross(north, m);
	float sR = sin(R);
	vec2 l = vec2(dot(d, right), dot(d, north)) / sR;
	float rr = length(l);
	float edge = clamp((1.0 - rr) * sR / pix + 0.5, 0.0, 1.0) * above;
	float limb = 0.80 + 0.20 * sqrt(clamp(1.0 - rr * rr, 0.0, 1.0));
	vec3 albedo = vec3(0.85);
	if (draw.p1.y > 0.5)
	{
		float lod = log2(max(pix / (2.0 * sR) * draw.p1.z, 1.0));
		albedo = textureLod(moonFace, vec2(0.5 + 0.5 * l.x, 0.5 - 0.5 * l.y), lod).rgb;
	}
	// The surface there, facing us: lit where it faces the sun, and the dark
	// part faintly by the earth's light.
	vec3 n = l.x * right + l.y * north - sqrt(max(1.0 - rr * rr, 0.0)) * m;
	float day = smoothstep(-0.04, 0.06, dot(n, s));
	vec3 col = albedo * limb * skyMoonColour.rgb * skyMoon.w * (day + skyMoonNorth.w * (1.0 - day));
	return min(mix(sky, behind, edge) + col * edge, vec3(1.0));
}

void main()
{
	vec3 d = normalize(Dir);

	// The panorama, by direction. Across its seam u jumps by 1: that is taken
	// out of the derivatives, or the seam draws as a line of the smallest mip.
	vec2 uv = PanoramaUV(d);
	vec2 dx = dFdx(uv), dy = dFdy(uv);
	dx.x -= floor(dx.x + 0.5);
	dy.x -= floor(dy.x + 0.5);
	vec3 sky = textureGrad(skyTexture, uv, dx, dy).rgb;
	if (draw.p0.w > 0.0)
		sky = mix(sky, textureGrad(nextTexture, uv, dx, dy).rgb, draw.p0.w);
	sky *= draw.p0.rgb;
	// The same with its stars blurred away, for behind the moon.
	float coarse = log2(float(textureSize(skyTexture, 0).x)) - 5.0;   // 32 texels round
	vec3 behind = textureLod(skyTexture, uv, coarse).rgb;
	if (draw.p0.w > 0.0)
		behind = mix(behind, textureLod(nextTexture, uv, coarse).rgb, draw.p0.w);
	behind = min(SkySunAir(behind * draw.p0.rgb, d), vec3(1.0));

	float pix = max(max(length(dFdx(d)), length(dFdy(d))), 1e-5);   // a pixel, radians

	sky = min(SkySunAir(sky, d), vec3(1.0));
	sky = SkyMoon(sky, behind, d, pix);

	// The sun: its glare in the eye and the air round it (the clouds put it back
	// in front of themselves, sky_clouds.frag), and its disc, a little darker at
	// its limb. The disc may be brighter than white, and bloom.
	if (skySun.w > 0.5)
	{
		vec3 s = normalize(skySun.xyz);
		float ang = 2.0 * asin(clamp(length(d - s) * 0.5, 0.0, 1.0));
		float glare = skySunShape.y * (0.9 * exp(-ang / 0.006) + 0.32 * exp(-ang / 0.035) + 0.10 * exp(-ang / 0.20));
		sky = min(sky + glare * skySunColour.rgb, vec3(1.0));
		float R = skySunShape.x;
		float edge = clamp((R - ang) / pix + 0.5, 0.0, 1.0) * smoothstep(-pix, pix, -d.y);
		float mu = sqrt(clamp(1.0 - (ang / R) * (ang / R), 0.0, 1.0));
		if (skySunColour.w > 0.0)
			sky = mix(sky, skySunColour.rgb * skySunColour.w * (0.75 + 0.25 * mu), edge);
	}

	color = vec4(TargetColor(sky), 1.0);
}
