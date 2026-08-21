#version 330

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;

out vec4 color;

uniform sampler2D theTexture;   // albedo (unit 0)
uniform sampler2D normalMap;    // tangent-space normal map (unit 1)

const int MAX_POINTS = 8;
const int MAX_SPOTS = 4;
const float PI = 3.14159265;

// Global fill so unlit areas aren't pure black (keep low for a dark room)
uniform vec3 ambientColor = vec3(0.08, 0.08, 0.10);

// Optional directional fill (the old "light" line); diffuse 0 = off
uniform vec3 dirLightDir = vec3(0.35, 1.0, 0.25);
uniform vec3 dirLightColor = vec3(1.0, 1.0, 1.0);
uniform float dirLightDiffuse = 0.0;

// Storm lightning: a brief bright flood of light from the sky (world up = -Y).
// lightningFlash pulses 0..~1 on each strike; 0 the rest of the time.
uniform float lightningFlash = 0.0;
uniform vec3  lightningColor = vec3(0.80, 0.85, 1.0);

// Light contributed by a lightning flash on a surface with normal N. Up-facing
// surfaces catch the most (the bolt lights the world from above); a flat fill
// keeps sides/undersides from staying black during the flash.
vec3 LightningLight(vec3 N)
{
    float up = clamp(-N.y, 0.0, 1.0);
    return lightningColor * lightningFlash * (0.35 + 0.65 * up);
}

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

// --- material ---
uniform vec3 viewPos = vec3(0.0);
uniform vec3 matTint = vec3(1.0);
uniform vec3 matEmissive = vec3(0.0);
uniform float matFresnel = 0.0;
uniform vec2 matUVTile = vec2(1.0);
uniform int matHasNormal = 0;
uniform float matNormalStrength = 1.0;
uniform int matNormalMode = 0;    // 0 = screen-space TBN, 1 = vertex tangents
uniform int matLighting = 0;      // 0 = Blinn-Phong, 1 = PBR
uniform float matSpecular = 0.2;  // Phong
uniform float matShininess = 24.0;
uniform float matMetallic = 0.0;  // PBR
uniform float matRoughness = 0.5;
uniform float matOpacity = 1.0;

// Seconds since start, for animated materials (matLighting == 2 water ripples).
uniform float uTime = 0.0;
// Water surface-ripple strength (per-surface; 1.0 = default detail).
uniform float uWaterChoppy = 1.0;

// Toon / cel shading: quantise the diffuse into flat bands + a hard specular.
// Overrides the lighting model when set (a global look, set by Scene3D).
uniform int toon = 0;

// --- shadow mapping (directional sun) ---
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
uniform float shadowStrength = 0.7;   // how much shadowed areas darken (0..1)
uniform int shadowsOn = 0;

// Returns 1.0 in full light, down to (1-strength) in shadow. PCF 3x3 + a
// slope-scaled depth bias to fight self-shadow acne.
float ShadowFactor(vec3 worldPos, float ndotl)
{
	if (shadowsOn == 0)
		return 1.0;
	vec4 lp = lightSpaceMatrix * vec4(worldPos, 1.0);
	vec3 p = lp.xyz / lp.w;
	p = p * 0.5 + 0.5;
	if (p.z > 1.0 || p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0)
		return 1.0;   // outside the light frustum -> lit
	float bias = max(0.0020 * (1.0 - ndotl), 0.0008);
	vec2 texel = 1.0 / vec2(textureSize(shadowMap, 0));
	float sh = 0.0;
	for (int x = -1; x <= 1; x++)
		for (int y = -1; y <= 1; y++)
			sh += (p.z - bias) > texture(shadowMap, p.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;
	sh /= 9.0;
	return 1.0 - shadowStrength * sh;
}

// --- point-light (cube) shadows, for indoor lamp-lit rooms ---
// On a GL 4.x desktop context (KINJO_GL4) a single cube-map ARRAY holds many
// casters, indexed dynamically by layer. On the 3.3/web fallback there are up to
// 4 separate cubes fetched with a constant index (GLSL 330 forbids dynamic
// sampler-array indexing).
uniform int pointShadowCount = 0;

#ifdef KINJO_GL4
const int MAX_PT_SHADOWS = 8;
uniform samplerCubeArray pointShadowArray;
uniform vec3 pointShadowPositions[MAX_PT_SHADOWS];
uniform float pointShadowFars[MAX_PT_SHADOWS];
uniform int pointShadowLightIdx[MAX_PT_SHADOWS];   // packed point-light index per caster

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
uniform int pointShadowLightIdx[MAX_PT_SHADOWS];   // packed point-light index per caster

// Constant-indexed cube fetch (GLSL 330 forbids dynamic sampler-array indexing).
float sampleShadowCube(int s, vec3 dir)
{
	if (s == 0) return texture(pointShadowMaps[0], dir).r;
	if (s == 1) return texture(pointShadowMaps[1], dir).r;
	if (s == 2) return texture(pointShadowMaps[2], dir).r;
	return texture(pointShadowMaps[3], dir).r;
}

// Shadow multiplier for point light `plIndex` (1.0 if it isn't a shadow caster).
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

// Finite-range falloff (reaches 0 at range): artist-friendly, no infinities
float Attenuate(float dist, float range)
{
	float a = clamp(1.0 - dist / range, 0.0, 1.0);
	return a * a;
}

// Screen-space tangent frame (no per-vertex tangents needed): builds T/B from
// the derivatives of world position + uv, so a normal map can perturb N.
mat3 CotangentFrame(vec3 N, vec3 p, vec2 uv)
{
	vec3 dp1 = dFdx(p);
	vec3 dp2 = dFdy(p);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);
	vec3 dp2perp = cross(dp2, N);
	vec3 dp1perp = cross(N, dp1);
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
	float invmax = inversesqrt(max(dot(T, T), dot(B, B)));
	return mat3(T * invmax, B * invmax, N);
}

// GGX / Trowbridge-Reitz normal distribution (PBR)
float DistributionGGX(vec3 N, vec3 H, float rough)
{
	float a = rough * rough;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
	return a2 / max(PI * d * d, 1e-5);
}

float GeometrySchlick(float NdotV, float rough)
{
	float r = rough + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

// Cel ramp: map a 0..1 lighting term to a few flat bands (~0.25 / 0.60 / 1.00).
float ToonRamp(float t)
{
	float a = smoothstep(0.24, 0.26, t);
	float b = smoothstep(0.59, 0.61, t);
	return 0.25 + a * 0.35 + b * 0.40;
}

// Accumulate one light's contribution using the active shading model.
void AddLight(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 albedo, vec3 F0,
              inout vec3 diffuseAccum, inout vec3 specularAccum)
{
	float NdotL = max(dot(N, L), 0.0);
	vec3 H = normalize(V + L);

	if (toon == 1)
	{
		diffuseAccum += albedo * radiance * ToonRamp(NdotL);
		float sp = pow(max(dot(N, H), 0.0), max(matShininess, 1.0));
		specularAccum += radiance * step(0.5, sp) * matSpecular;   // hard highlight
	}
	else if (matLighting == 1)   // PBR (Cook-Torrance)
	{
		float NDF = DistributionGGX(N, H, matRoughness);
		float G = GeometrySchlick(max(dot(N, V), 0.0), matRoughness) *
		          GeometrySchlick(NdotL, matRoughness);
		vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
		vec3 spec = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * NdotL, 1e-4);
		vec3 kd = (vec3(1.0) - F) * (1.0 - matMetallic);
		diffuseAccum += kd * albedo / PI * radiance * NdotL;
		specularAccum += spec * radiance * NdotL;
	}
	else                          // Blinn-Phong
	{
		float s = pow(max(dot(N, H), 0.0), matShininess) * matSpecular;
		diffuseAccum += albedo * radiance * NdotL;
		specularAccum += radiance * s * NdotL;
	}
}

void main()
{
	vec2 uv = TexCoord * matUVTile;

	vec4 texColor = texture(theTexture, uv);
	if (texColor.a < 0.1)
		discard;

	vec3 albedo = texColor.rgb * matTint;

	// --- animated water surface -------------------------------------------
	// A special mode for lake/sea planes: procedural sine ripples perturb the
	// normal, a fresnel term blends the deep-water body colour into a reflected
	// sky colour at grazing angles, and sharp specular gives moving sun glints.
	// Responds to the scene's ambient + directional light, so it matches the
	// time of day. Ignores toon/cel (water should read as smooth + glossy).
	if (matLighting == 2)
	{
		vec3 Ngeo = normalize(Normal);
		vec3 Vw = normalize(viewPos - FragPos);
		vec2 wp = FragPos.xz;

		// Four directional sine waves; accumulate the analytic world-XZ slope.
		vec4 waves[4] = vec4[4](
			vec4( 1.00,  0.35, 0.010, 1.1),
			vec4(-0.60,  1.00, 0.014, 1.5),
			vec4( 0.80, -0.75, 0.021, 1.9),
			vec4(-0.40, -0.55, 0.032, 2.4));
		float amps[4] = float[4](1.0, 0.7, 0.45, 0.28);
		float slopeX = 0.0;
		float slopeZ = 0.0;
		for (int i = 0; i < 4; i++)
		{
			vec2 d = normalize(waves[i].xy);
			float freq = waves[i].z;
			float phase = (d.x * wp.x + d.y * wp.y) * freq + uTime * waves[i].w;
			float c = cos(phase) * amps[i] * freq;
			slopeX += d.x * c;
			slopeZ += d.y * c;
		}
		vec3 Nw = normalize(Ngeo + vec3(-slopeX, 0.0, -slopeZ) * 30.0 * uWaterChoppy);

		// Grazing angles reflect the sky; steep angles show the water body.
		float fres = pow(1.0 - max(dot(Nw, Vw), 0.0), 3.0);
		fres = clamp(0.08 + 0.92 * fres, 0.0, 1.0);

		vec3 sceneLit = ambientColor + dirLightColor * dirLightDiffuse
			+ lightningColor * lightningFlash * 0.9;   // storm flash brightens the water
		vec3 deep    = albedo * (0.35 + sceneLit);
		vec3 skyRefl = (0.35 + sceneLit) * vec3(0.55, 0.70, 0.92);
		vec3 wcol = mix(deep, skyRefl, fres);

		// Moving sun/lamp glints (sharp specular off the rippled normal).
		vec3 glint = vec3(0.0);
		if (dirLightDiffuse > 0.0)
		{
			vec3 H = normalize(Vw + normalize(-dirLightDir));
			glint += dirLightColor * pow(max(dot(Nw, H), 0.0), matShininess) * matSpecular;
		}
		for (int i = 0; i < pointCount; i++)
		{
			vec3 Lv = pointPos[i] - FragPos;
			float dist = length(Lv);
			if (dist < 0.0001) continue;
			vec3 H = normalize(Vw + Lv / dist);
			glint += pointColor[i] * pointIntensity[i] * Attenuate(dist, pointRange[i])
			         * pow(max(dot(Nw, H), 0.0), matShininess) * matSpecular;
		}

		float a = clamp(matOpacity + fres * 0.35 + max(glint.r, max(glint.g, glint.b)), 0.0, 1.0);
		color = vec4(wcol + glint, a);
		return;
	}

	// Base geometric normal, optionally perturbed by the normal map. The TBN
	// frame comes either from real vertex tangents (matNormalMode == 1) or is
	// reconstructed from screen-space derivatives (0).
	vec3 N = normalize(Normal);
	if (matHasNormal == 1)
	{
		vec3 mapN = texture(normalMap, uv).rgb * 2.0 - 1.0;
		mapN.xy *= matNormalStrength;
		mat3 TBN;
		if (matNormalMode == 1)
		{
			vec3 T = normalize(Tangent - N * dot(N, Tangent));  // Gram-Schmidt
			vec3 B = cross(N, T);
			TBN = mat3(T, B, N);
		}
		else
		{
			TBN = CotangentFrame(N, FragPos, uv);
		}
		N = normalize(TBN * mapN);
	}

	vec3 V = normalize(viewPos - FragPos);

	vec3 diffuseAccum = vec3(0.0);
	vec3 specularAccum = vec3(0.0);
	vec3 F0 = mix(vec3(0.04), albedo, matMetallic);   // PBR base reflectance

	// Directional fill (usually off in a point/spot-lit room)
	if (dirLightDiffuse > 0.0)
		AddLight(normalize(-dirLightDir), dirLightColor * dirLightDiffuse,
			N, V, albedo, F0, diffuseAccum, specularAccum);

	// Point lights
	for (int i = 0; i < pointCount; i++)
	{
		vec3 Lv = pointPos[i] - FragPos;
		float dist = length(Lv);
		if (dist < 0.0001) continue;
		vec3 radiance = pointColor[i] * pointIntensity[i] * Attenuate(dist, pointRange[i]);
		if (pointShadowCount > 0)
			radiance *= PointShadowForLight(i, FragPos);
		AddLight(Lv / dist, radiance, N, V, albedo, F0, diffuseAccum, specularAccum);
	}

	// Spot lights (distance falloff * cone falloff)
	for (int i = 0; i < spotCount; i++)
	{
		vec3 Lv = spotPos[i] - FragPos;
		float dist = length(Lv);
		if (dist < 0.0001) continue;
		vec3 L = Lv / dist;
		float theta = dot(-L, normalize(spotDir[i]));
		float cone = smoothstep(spotCosOuter[i], spotCosInner[i], theta);
		vec3 radiance = spotColor[i] * spotIntensity[i] * Attenuate(dist, spotRange[i]) * cone;
		AddLight(L, radiance, N, V, albedo, F0, diffuseAccum, specularAccum);
	}

	// Sun shadow: darken the direct (diffuse+specular) contribution where the
	// fragment is occluded from the sun; ambient still fills shadowed areas.
	float ndotl = max(dot(N, normalize(-dirLightDir)), 0.0);
	float shadow = ShadowFactor(FragPos, ndotl);

	// Cel shading flattens the ambient into the shadow band too.
	vec3 ambient = albedo * ambientColor;
	vec3 lit = ambient + (diffuseAccum + specularAccum) * shadow;

	// Storm lightning floods the surface with a brief sky-lit burst.
	lit += albedo * LightningLight(N);

	// Fresnel rim (ice/glass, and a cheap cel rim light): grazing-angle glow.
	if (matFresnel > 0.0)
	{
		float f = pow(1.0 - max(dot(N, V), 0.0), 5.0);
		if (toon == 1)
			f = step(0.6, f);   // hard rim
		lit += matFresnel * f * vec3(1.0);
	}

	lit += matEmissive;

	color = vec4(lit, texColor.a * matOpacity);
}
