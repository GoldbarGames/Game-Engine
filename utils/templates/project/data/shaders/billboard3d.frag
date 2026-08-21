#version 330

in vec2 TexCoord;
in vec3 FragPos;

layout(location = 0) out vec4 color;
// "Is-character" mask for the toon outline (written only when the framebuffer's
// draw buffer 1 is enabled; discarded otherwise). .r = 1 marks a character; .a
// carries the sprite's coverage so alpha blending (which is on for billboards)
// weights the mask by opacity instead of zeroing it out on a lone float output.
layout(location = 1) out vec4 oMask;

uniform sampler2D theTexture;

const int MAX_POINTS = 8;
const int MAX_SPOTS = 4;

// VN character art is already shaded, so billboards are not lit per-normal.
// Instead the art is treated as albedo and multiplied by how much light
// reaches the fragment's world position - so a character standing in a
// spotlight is bright and one in shadow goes dark, matching the room.
uniform vec3 ambientColor = vec3(0.08, 0.08, 0.10);

uniform vec3 dirLightColor = vec3(1.0, 1.0, 1.0);
uniform float dirLightDiffuse = 0.0;

// Storm lightning: a brief bright flood of light on the character during a strike.
uniform float lightningFlash = 0.0;
uniform vec3  lightningColor = vec3(0.80, 0.85, 1.0);

uniform int pointCount = 0;
uniform vec3 pointPos[MAX_POINTS];
uniform vec3 pointColor[MAX_POINTS];
uniform float pointRange[MAX_POINTS];
uniform float pointIntensity[MAX_POINTS];

uniform int spotCount = 0;
uniform vec3 spotPos[MAX_SPOTS];
uniform vec3 spotDir[MAX_SPOTS];
uniform vec3 spotColor[MAX_SPOTS];
uniform float spotRange[MAX_SPOTS];
uniform float spotIntensity[MAX_SPOTS];
uniform float spotCosInner[MAX_SPOTS];
uniform float spotCosOuter[MAX_SPOTS];

float Attenuate(float dist, float range)
{
	float a = clamp(1.0 - dist / range, 0.0, 1.0);
	return a * a;
}

// --- shadow mapping (directional sun) ---
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform float shadowStrength = 0.7;
uniform int shadowsOn = 0;

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
// GL 4.x desktop: one cube-map array (dynamic layer index). 3.3/web: up to 4
// separate cubes, constant-indexed. See scene3d.frag for the full rationale.
uniform int pointShadowCount = 0;

#ifdef KINJO_GL4
const int MAX_PT_SHADOWS = 8;
uniform samplerCubeArray pointShadowArray;
uniform vec3 pointShadowPositions[MAX_PT_SHADOWS];
uniform float pointShadowFars[MAX_PT_SHADOWS];
uniform int pointShadowLightIdx[MAX_PT_SHADOWS];

float PointShadowForLight(int plIndex, vec3 worldPos)
{
	for (int s = 0; s < MAX_PT_SHADOWS; s++)
	{
		if (s >= pointShadowCount) break;
		if (pointShadowLightIdx[s] != plIndex) continue;
		vec3 f2l = worldPos - pointShadowPositions[s];
		float cur = length(f2l);
		float closest = texture(pointShadowArray, vec4(f2l, float(s))).r * pointShadowFars[s];
		float bias = max(0.03 * cur, 5.0);
		float lit = (cur - bias > closest) ? 0.0 : 1.0;
		return 1.0 - shadowStrength * (1.0 - lit);
	}
	return 1.0;
}
#else
const int MAX_PT_SHADOWS = 4;
uniform samplerCube pointShadowMaps[MAX_PT_SHADOWS];
uniform vec3 pointShadowPositions[MAX_PT_SHADOWS];
uniform float pointShadowFars[MAX_PT_SHADOWS];
uniform int pointShadowLightIdx[MAX_PT_SHADOWS];

float sampleShadowCube(int s, vec3 dir)
{
	if (s == 0) return texture(pointShadowMaps[0], dir).r;
	if (s == 1) return texture(pointShadowMaps[1], dir).r;
	if (s == 2) return texture(pointShadowMaps[2], dir).r;
	return texture(pointShadowMaps[3], dir).r;
}

float PointShadowForLight(int plIndex, vec3 worldPos)
{
	for (int s = 0; s < MAX_PT_SHADOWS; s++)
	{
		if (s >= pointShadowCount) break;
		if (pointShadowLightIdx[s] != plIndex) continue;
		vec3 f2l = worldPos - pointShadowPositions[s];
		float cur = length(f2l);
		float closest = sampleShadowCube(s, f2l) * pointShadowFars[s];
		float bias = max(0.03 * cur, 5.0);
		float lit = (cur - bias > closest) ? 0.0 : 1.0;
		return 1.0 - shadowStrength * (1.0 - lit);
	}
	return 1.0;
}
#endif

void main()
{
	vec4 c = texture(theTexture, TexCoord);
	if (c.a < 0.1)
		discard;

	// Flat fill (no normal term for pre-shaded billboards). The sun contribution
	// is darkened where the character is in shadow; ambient still fills it.
	float shadow = ShadowFactor(FragPos);
	vec3 light = ambientColor + dirLightColor * dirLightDiffuse * shadow;

	// Storm lightning floods the character with a brief sky-lit burst.
	light += lightningColor * lightningFlash;

	for (int i = 0; i < pointCount; i++)
	{
		float dist = length(pointPos[i] - FragPos);
		vec3 radiance = pointColor[i] * pointIntensity[i] * Attenuate(dist, pointRange[i]);
		if (pointShadowCount > 0)
			radiance *= PointShadowForLight(i, FragPos);
		light += radiance;
	}

	for (int i = 0; i < spotCount; i++)
	{
		vec3 d = spotPos[i] - FragPos;
		float dist = length(d);
		if (dist < 0.0001) continue;
		vec3 L = d / dist;
		float att = Attenuate(dist, spotRange[i]);
		float theta = dot(-L, normalize(spotDir[i]));
		float cone = smoothstep(spotCosOuter[i], spotCosInner[i], theta);
		light += spotColor[i] * spotIntensity[i] * att * cone;
	}

	color = vec4(c.rgb * light, c.a);
	oMask = vec4(1.0, 0.0, 0.0, c.a);   // character; alpha = coverage for blending
}
