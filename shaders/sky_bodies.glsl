#ifndef KINJO_SKY_BODIES_GLSL
#define KINJO_SKY_BODIES_GLSL
// THE SUN AND THE MOON (src/ENGINE/SkyBodies.h): the block SkyBodies.cpp fills,
// and the sun's light in the air, which both the sky (sky.frag) and image-based
// lighting's capture of it (env_capture.frag) add to the panorama. Colours are
// sRGB, in the panorama's own terms. Directions are world, up is -Y.

layout(std140) uniform Sky
{
	vec4 skySun;          // toward the sun xyz; w: 1 when a game has set it
	vec4 skySunColour;    // the disc and its glare, rgb; w: the disc's brightness
	vec4 skySunShape;     // the disc's angular radius, glare, wide glow's falloff, near glow's falloff
	vec4 skyGlow;         // the sky's glow toward the sun, rgb; w: wide
	vec4 skyGlow2;        // near, horizon amount, dusk, -
	vec4 skyHorizon;      // the horizon under the sun, rgb
	vec4 skyMoon;         // toward the moon xyz; w: brightness (0: none)
	vec4 skyMoonColour;   // rgb; w: angular radius
	vec4 skyMoonNorth;    // toward the celestial pole xyz; w: earthshine
	vec4 skyMoonGlow;     // rgb; w: glow
};

// The sky's colour `sky` looking along `d`, with the sun's light in the air: the
// glow round it, and at dawn and dusk the warm horizon under it and, low on the
// far side, the pink band (the belt of Venus) over the earth's blue shadow.
vec3 SkySunAir(vec3 sky, vec3 d)
{
	if (skySun.w < 0.5)
		return sky;
	vec3 s = normalize(skySun.xyz);
	float up = max(-d.y, 0.0);
	float g = pow(up, 0.45);                 // the panoramas' gradient: horizon 0, zenith 1
	// +1 looking toward the sun's bearing, -1 away from it.
	float fl = length(d.xz), sl = length(s.xz);
	float a = (fl > 1e-4 && sl > 1e-4) ? dot(d.xz / fl, s.xz / sl) : 0.0;
	float toward = (a + 1.0) * 0.5;
	toward *= toward;
	float away = (1.0 - a) * 0.5;
	away *= away;

	sky = mix(sky, skyHorizon.rgb, clamp(skyGlow2.y * toward * (1.0 - g), 0.0, 1.0));
	float dusk = skyGlow2.z * away;
	float b = (up - 0.12) / 0.05;
	sky += dusk * exp(-b * b) * vec3(0.26, 0.10, 0.14);
	float e = up / 0.05;
	sky = mix(sky, vec3(0.32, 0.32, 0.46), 0.30 * dusk * exp(-e * e));

	float dd = dot(d, s);
	float glow = skyGlow.w * exp((dd - 1.0) * skySunShape.z) + skyGlow2.x * exp((dd - 1.0) * skySunShape.w);
	return sky + glow * (1.0 - 0.5 * g) * skyGlow.rgb;
}
#endif
