#ifndef KINJO_POINT_SHADOWS_GLSL
#define KINJO_POINT_SHADOWS_GLSL
// Point-light (cube) shadows, for indoor lamp-lit rooms. Shared by the lit
// shaders (scene3d.frag, billboard3d.frag) and the volumetric fog.
//
// On a GL 4.x desktop context (KINJO_GL4) a single cube-map ARRAY holds many
// casters, indexed dynamically by layer. On the 3.3/web fallback there are up to
// 4 separate cubes fetched with a constant index (GLSL 330 forbids dynamic
// sampler-array indexing). Caster positions and far planes are in the Scene
// block; each light knows its caster index (SceneLight.shadow, lights.glsl).
// Include after scene.glsl.

#ifdef KINJO_GL4
const int MAX_PT_SHADOWS = 8;
uniform samplerCubeArray pointShadowArray;

// Shadow multiplier from point-shadow caster `s`.
float PointShadowSlot(int s, vec3 worldPos)
{
	vec3 f2l = worldPos - pointShadowPositions[s];
	float cur = length(f2l);
	float closest = texture(pointShadowArray, vec4(f2l, float(s))).r * pointShadowFars[s];
	float bias = max(0.03 * cur, 5.0);
	float lit = (cur - bias > closest) ? 0.0 : 1.0;
	return 1.0 - shadowStrength * (1.0 - lit);
}
#else
const int MAX_PT_SHADOWS = 4;
uniform samplerCube pointShadowMaps[MAX_PT_SHADOWS];

// Constant-indexed cube fetch (GLSL 330 forbids dynamic sampler-array indexing).
float sampleShadowCube(int s, vec3 dir)
{
	if (s == 0) return texture(pointShadowMaps[0], dir).r;
	if (s == 1) return texture(pointShadowMaps[1], dir).r;
	if (s == 2) return texture(pointShadowMaps[2], dir).r;
	return texture(pointShadowMaps[3], dir).r;
}

// Shadow multiplier from point-shadow caster `s`.
float PointShadowSlot(int s, vec3 worldPos)
{
	vec3 f2l = worldPos - pointShadowPositions[s];
	float cur = length(f2l);
	float closest = sampleShadowCube(s, f2l) * pointShadowFars[s];
	float bias = max(0.03 * cur, 5.0);
	float lit = (cur - bias > closest) ? 0.0 : 1.0;
	return 1.0 - shadowStrength * (1.0 - lit);
}
#endif
#endif
