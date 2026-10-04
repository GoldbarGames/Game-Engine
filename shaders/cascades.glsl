#ifndef KINJO_CASCADES_GLSL
#define KINJO_CASCADES_GLSL
// Cascaded sun shadows (Phase 1.5 item 5; src/ENGINE/Scene3DShadows.cpp).
// Up to 4 shadow maps, each covering a slice of the camera's view by a
// bounding sphere: sharp near the camera, reaching far into the distance.
// The maps compare in hardware (sampler2DArrayShadow), so every PCF tap is
// already a bilinear-filtered visibility.

layout(std140) uniform Cascades
{
	mat4  cascadeViewProj[4];   // light projection * light view, per cascade
	vec4  cascadeSphere[4];     // xyz centre, w radius^2 (the region it covers)
	vec4  cascadeTexelWorld;    // world size of one texel, per cascade
	vec4  cascadeDepthBias;     // constant depth bias in map units, per cascade
	int   cascadeCount;         // 0 = no sun shadow this frame
	float cascadeSoftness;      // PCF kernel spacing, in texels
	float cascadePad0;
	float cascadePad1;
};

uniform sampler2DArrayShadow shadowCascades;   // unit 11

// Visibility (1 = lit) of worldPos in cascade i: a 4x4 grid of hardware-
// filtered taps, after nudging the point out along its normal (normal-offset
// bias: no acne, and far less peter-panning than depth bias). The nudge grows
// as the surface turns edge-on to the sun: a face lit at a grazing angle lies
// within a filter-width of the surface above it in the map and would
// otherwise shadow itself.
float CascadeVisibility(int i, vec3 worldPos, vec3 N, float NdotL)
{
	float texel = cascadeTexelWorld[i];
	vec3 p = worldPos + N * (texel * (1.0 + 3.0 * (1.0 - NdotL)));
	vec4 lp = cascadeViewProj[i] * vec4(p, 1.0);
	vec3 sc = lp.xyz / lp.w * 0.5 + 0.5;
	float ref = sc.z - cascadeDepthBias[i];
	vec2 tapStep = cascadeSoftness / vec2(textureSize(shadowCascades, 0).xy);
	float sum = 0.0;
	for (int y = 0; y < 4; y++)
	{
		for (int x = 0; x < 4; x++)
		{
			vec2 o = (vec2(float(x), float(y)) - 1.5) * tapStep;
			sum += texture(shadowCascades, vec4(sc.xy + o, float(i), ref));
		}
	}
	return sum / 16.0;
}

// Sun visibility of a point in the air (volumetric fog): one hardware-filtered
// tap in the sharpest cascade that holds it, no normal offset. Cheap enough to
// take at every step of a ray march.
float SunShadowVolume(vec3 worldPos)
{
	for (int i = 0; i < 4; i++)
	{
		if (i >= cascadeCount)
			break;
		vec3 d = worldPos - cascadeSphere[i].xyz;
		if (dot(d, d) >= cascadeSphere[i].w)
			continue;
		vec4 lp = cascadeViewProj[i] * vec4(worldPos, 1.0);
		vec3 sc = lp.xyz / lp.w * 0.5 + 0.5;
		return texture(shadowCascades, vec4(sc.xy, float(i), sc.z - cascadeDepthBias[i]));
	}
	return 1.0;
}

// Sun visibility at worldPos (1 = lit), for a surface with normal N under
// sunlight travelling along lightDir. Picks the first (sharpest) cascade whose
// sphere contains the point, blends into the next one across the outer part of
// its sphere, and fades the shadow out at the edge of the last.
float SunShadow(vec3 worldPos, vec3 N, vec3 lightDir)
{
	float NdotL = clamp(dot(N, -lightDir), 0.0, 1.0);
	for (int i = 0; i < 4; i++)
	{
		if (i >= cascadeCount)
			break;
		vec3 d = worldPos - cascadeSphere[i].xyz;
		float edge = dot(d, d) / cascadeSphere[i].w;   // 0 at the centre, 1 at the edge
		if (edge >= 1.0)
			continue;
		float vis = CascadeVisibility(i, worldPos, N, NdotL);
		if (edge > 0.8)
		{
			float t = (edge - 0.8) / 0.2;
			float beyond = (i + 1 < cascadeCount) ? CascadeVisibility(i + 1, worldPos, N, NdotL) : 1.0;
			vis = mix(vis, beyond, t);
		}
		return vis;
	}
	return 1.0;
}
#endif
