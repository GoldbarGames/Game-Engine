#version 330

in vec2 TexCoord;
in vec3 FragPos;
in vec3 PrevWorldPos;

layout(location = 0) out vec4 color;
// "Is-character" mask for the toon outline (written only when the framebuffer's
// draw buffer 1 is enabled; discarded otherwise). .r = 1 marks a character; .a
// carries the sprite's coverage so alpha blending (which is on for billboards)
// weights the mask by opacity instead of zeroing it out on a lone float output.
layout(location = 1) out vec4 oMask;
// Motion vectors for temporal anti-aliasing (motion.glsl), stored only while
// the engine enables the world target's attachment 2.
layout(location = 2) out vec4 oMotion;

uniform sampler2D theTexture;

// VN character art is already shaded, so billboards are not lit per-normal.
// Instead the art is treated as albedo and multiplied by how much light
// reaches the fragment's world position - so a character standing in a
// spotlight is bright and one in shadow goes dark, matching the room.
// Ambient, sun, lightning, point/spot lights and shadow data: Scene block.
#include "scene.glsl"
// Linear-aware: character art is an sRGB texture and the block's light colours
// arrive linear in a linear-workflow project (render/ColorPipeline.h).
#include "target.glsl"
#include "environment.glsl"   // the sky's ambient, when image-based lighting is on
#include "cascades.glsl"      // cascaded sun shadows (cascadeCount = 0 when unused)
#include "motion.glsl"
#include "camera.glsl"
#include "lights.glsl"        // point and spot lights, clustered (LightRange / GetLight)
#include "distance_fog.glsl"  // ApplyDistanceFog (distFogParams.z = 0 when off)

float Attenuate(float dist, float range)
{
	float a = clamp(1.0 - dist / range, 0.0, 1.0);
	return a * a;
}

// --- shadow mapping (directional sun) ---
uniform sampler2D shadowMap;

float ShadowFactor(vec3 worldPos)
{
	if (shadowsOn == 0)
		return 1.0;
	vec4 lp = lightSpaceMatrix * vec4(worldPos, 1.0);
	vec3 p = lp.xyz / lp.w;
	p = p * 0.5 + 0.5;
	if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
		return 1.0;
	float bias = 0.0012;
	vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
	float sh = 0.0;
	for (int x = -1; x <= 1; x++)
		for (int y = -1; y <= 1; y++)
			sh += (p.z - bias) > texture(shadowMap, p.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;
	sh /= 9.0;
	return 1.0 - shadowStrength * sh;
}

// --- point-light (cube) shadows ---
#include "point_shadows.glsl"   // PointShadowSlot (cube shadows of the lamps)

void main()
{
	vec4 c = texture(theTexture, TexCoord);
	if (c.a < 0.1)
		discard;

	// Flat fill (no normal term for pre-shaded billboards). The sun contribution
	// is darkened where the character is in shadow; ambient still fills it.
	// Cascaded maps when rendered (offset toward the camera, the way the quad
	// faces), else the single sun map.
	vec3 facing = vec3(viewPos.x - FragPos.x, 0.0, viewPos.z - FragPos.z);
	float shadow = (cascadeCount > 0)
		? 1.0 - shadowStrength * (1.0 - SunShadow(FragPos, normalize(facing + vec3(0.0, 0.0, 1e-4)), normalize(dirLightDir)))
		: ShadowFactor(FragPos);
	vec3 light = ((iblOn != 0) ? EnvDiffuseFlat() : ambientColor) + dirLightColor * dirLightDiffuse * shadow;

	// Storm lightning floods the character with a brief sky-lit burst.
	light += lightningColor * lightningFlash;

	// Point and spot lights: the ones that reach this pixel's cluster
	// (lights.glsl), points first, then spots.
	int lightFirst, lightCount;
	LightRange(FragPos, lightFirst, lightCount);
	for (int k = lightFirst; k < lightFirst + lightCount; k++)
	{
		SceneLight l = GetLight(k);
		if (l.spot == 0)
		{
			float dist = length(l.pos - FragPos);
			vec3 radiance = l.color * l.intensity * Attenuate(dist, l.range);
			if (l.shadow >= 0)
				radiance *= PointShadowSlot(l.shadow, FragPos);
			light += radiance;
		}
		else
		{
			vec3 d = l.pos - FragPos;
			float dist = length(d);
			if (dist < 0.0001) continue;
			vec3 L = d / dist;
			float att = Attenuate(dist, l.range);
			float theta = dot(-L, normalize(l.dir));
			float cone = smoothstep(l.cosOuter, l.cosInner, theta);
			light += l.color * l.intensity * att * cone;
		}
	}

	color = vec4(ApplyDistanceFog(c.rgb * light, FragPos), c.a);
	oMask = vec4(1.0, 0.0, 0.0, c.a);   // character; alpha = coverage for blending
	oMotion = MotionVector(FragPos, PrevWorldPos, step(0.5, c.a));
}
