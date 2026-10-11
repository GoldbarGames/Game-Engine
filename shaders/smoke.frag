#version 330 core

// Smoke and steam (src/ENGINE/Smoke.cpp, docs/VISUAL_EFFECTS.md).
//
// Each puff stands for a soft ball of smoke. Its billow texture gives the
// density; the ball it stands for gives a normal, so a puff is lit brighter on
// its sunny side and darker on the far side, the more so the thicker it is.
// Steam scatters sunlight forward: looked at against the sun it glows. The sky
// lights it from all round, the scene's lamps by their distance, and the sun
// only where the sun's shadow maps say the sun reaches.
//
// Written premultiplied (colour times coverage): blended with
// BlendMode::Premultiplied, so a glowing spark adds light even where it is
// nearly transparent.

in vec2 vCorner;
in vec2 vTex;
in vec3 vWorld;
flat in vec3 vCenter;
flat in float vRadius;
flat in vec4 vParams;    // rotation, opacity, steam, seed
flat in vec4 vColor;     // albedo (authored sRGB), glow

#include "camera.glsl"
#include "scene.glsl"
#include "target.glsl"
#include "environment.glsl"
#include "cascades.glsl"
#include "lights.glsl"
#include "distance_fog.glsl"
#include "smoke_draw.glsl"

uniform sampler2D puffTex;      // unit 0: the billows (density in r)
uniform sampler2D sceneDepth;   // unit 1: the world's depth, when draw.uDepth.z = 1

layout(location = 0) out vec4 color;

// A depth-buffer value as a distance along the view axis.
float ViewDepth(float d)
{
	float n = draw.uDepth.x, f = draw.uDepth.y;
	return (2.0 * n * f) / (f + n - (d * 2.0 - 1.0) * (f - n));
}

float Attenuate(float dist, float range)
{
	float a = clamp(1.0 - dist / range, 0.0, 1.0);
	return a * a;
}

void main()
{
	float r2 = dot(vCorner, vCorner);
	if (r2 >= 1.0)
		discard;
	float density = texture(puffTex, vTex).r;
	float alpha = density * vParams.y;
	if (alpha < 0.002)
		discard;

	// The ball this puff stands for, as seen from the eye.
	vec3 right = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 up    = vec3(view[0][1], view[1][1], view[2][1]);
	vec3 toEye = normalize(viewPos - vCenter);
	float bulge = sqrt(1.0 - r2);
	// ...with the billow's lumps on it: the slope of its density, turned back
	// from the texture's rotation into the puff's frame, tilts the normal, so
	// the sun picks out each lump and shades the hollows between.
	const float STEP = 1.5 / 256.0;
	float gx = texture(puffTex, vTex + vec2(STEP, 0.0)).r - texture(puffTex, vTex - vec2(STEP, 0.0)).r;
	float gy = texture(puffTex, vTex + vec2(0.0, STEP)).r - texture(puffTex, vTex - vec2(0.0, STEP)).r;
	float ca = cos(vParams.x), sa = sin(vParams.x);
	vec2 slope = vec2(gx * ca + gy * sa, -gx * sa + gy * ca) * 2.5;
	// Flatter than a true ball: smoke is lit through, not on a surface, and a
	// full ball's rim (facing the sky) lit up brighter than its middle, so
	// every puff looked like a bubble.
	vec3 n = normalize(right * (vCorner.x - slope.x) + up * (vCorner.y - slope.y) + toEye * (bulge + 0.8));

	// Where the world is in front of it, none of it; where the world is just
	// behind it, thinning to nothing, so it meets the ground or a boiler in a
	// soft line. Measured from the ball's near surface, not the flat card.
	float eye = ViewDepth(gl_FragCoord.z);
	if (draw.uDepth.z > 0.5)
	{
		float scene = ViewDepth(texelFetch(sceneDepth, ivec2(gl_FragCoord.xy), 0).r);
		float front = eye - bulge * vRadius * 0.5;
		alpha *= clamp((scene - front) / max(draw.uDepth.w * vRadius, 0.05), 0.0, 1.0);
	}
	// With the eye inside a puff, its card would cut across the view: it
	// thins out instead.
	alpha *= smoothstep(draw.uFade.x * vRadius, draw.uFade.y * vRadius, eye);
	if (alpha < 0.002)
		discard;

	float steam = vParams.z;
	vec3 L = normalize(-dirLightDir);
	vec3 inside = vCenter + n * vRadius * 0.5;

	// The sun, where it reaches: on the lit side of the ball, less on the far
	// side the thicker the puff is.
	float sunVisible = (cascadeCount > 0) ? 1.0 - shadowStrength * (1.0 - SunShadowVolume(inside)) : 1.0;
	vec3 sun = dirLightColor * dirLightDiffuse * sunVisible;
	float wrap = clamp(dot(n, L) * 0.5 + 0.5, 0.0, 1.0);
	float self = mix(1.0, wrap * wrap, clamp(0.3 + 0.6 * density, 0.0, 1.0));
	vec3 light = sun * self;

	// Forward scattering (Henyey-Greenstein): thin steam between the eye and
	// the sun lights up.
	float g = 0.6;
	float cosT = dot(-toEye, L);
	float hg = (1.0 - g * g) / pow(max(1.0 + g * g - 2.0 * g * cosT, 1e-4), 1.5);
	light += sun * hg * 0.35 * steam * (1.0 - 0.5 * density);

	// The sky, mostly from above (it lights smoke through and through), a
	// little from the side it faces; the storm's flash.
	vec3 skyward = normalize(n * 0.5 + vec3(0.0, -1.0, 0.0));   // up is -Y
	light += (iblOn != 0) ? EnvDiffuse(skyward) : ambientColor;
	light += lightningColor * lightningFlash;

	// The scene's lamps: a headlamp shining through the exhaust.
	int lightFirst, lightCount;
	LightRange(vWorld, lightFirst, lightCount);
	for (int k = lightFirst; k < lightFirst + lightCount; k++)
	{
		SceneLight l = GetLight(k);
		vec3 d = l.pos - inside;
		float dist = length(d);
		if (dist < 0.0001)
			continue;
		float att = Attenuate(dist, l.range);
		if (l.spot != 0)
			att *= smoothstep(l.cosOuter, l.cosInner, dot(-d / dist, normalize(l.dir)));
		light += l.color * l.intensity * att * (0.5 + 0.5 * steam);
	}

	vec3 albedo = TargetColor(vColor.rgb);
	vec3 c = albedo * light + albedo * vColor.a;
	c = ApplyDistanceFog(c, vWorld);
	color = vec4(c * alpha, alpha);
}
