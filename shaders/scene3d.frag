#version 330

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec3 Tangent;
in vec3 PrevWorldPos;

layout(location = 0) out vec4 color;
// Motion vectors for temporal anti-aliasing (motion.glsl). Only stored while
// the engine enables the world target's attachment 2 for this draw.
layout(location = 2) out vec4 oMotion;

uniform sampler2D theTexture;   // albedo (unit 0)
uniform sampler2D normalMap;    // tangent-space normal map (unit 1)
// Maps of a material imported with a model (glTF; Material.matMaps says which).
uniform sampler2D metallicRoughnessMap;   // unit 2: G = roughness, B = metallic (R = occlusion if packed)
#ifdef KINJO_GL4
uniform sampler2D occlusionMap;           // unit 5 (a point-shadow cube's unit on the 3.3/web fallback)
uniform sampler2D emissiveMap;            // unit 6
#endif

// This pixel's metallic and roughness: the material's, times its
// metallic-roughness map when it has one. Set at the top of main().
float surfMetallic;
float surfRoughness;

const float PI = 3.14159265;

// Lights, shadows, camera position, time, toon flag (Scene block) and the
// surface material (Material block).
#include "scene.glsl"
#include "material.glsl"
// Linear-aware (render/ColorPipeline.h): in a linear-workflow project the
// albedo texture is sRGB (sampled as linear) and the block colours arrive
// linear, so the lighting here is computed and written in linear space.
#include "target.glsl"
#include "environment.glsl"   // image-based lighting from the sky (iblOn = 0 when unused)
#include "cascades.glsl"      // cascaded sun shadows (cascadeCount = 0 when unused)
#include "ao.glsl"            // screen-space ambient occlusion (aoOn = 0 when unused)
#include "motion.glsl"        // motion vectors (motionOn = 0 when unused)
#include "camera.glsl"
#include "lights.glsl"        // point and spot lights, clustered (LightRange / GetLight)

// --- ground splatting (MAT_SPLAT) ---------------------------------------------
// A ground material (materials.txt `splat`) lays three more textures over its
// own - rock, earth, snow, whatever the game gives it. Each vertex says how
// much of each in its tangent slot (x, y, z; the material's own texture takes
// what is left). Each texture's detail, measured against its own average,
// sharpens the hand-off - a cheap height blend, so rock shows through grass
// along its cracks first rather than as a fade.
//
// GLSL 4.20+ only: the array's unit is fixed here, because a sampler2DArray
// left on unit 0 beside the albedo's sampler2D would stop every draw; and on
// the 3.3/web fallback unit 7 holds a point-shadow cube.
#if defined(KINJO_GL4) && __VERSION__ >= 420
#define KINJO_SPLAT
layout(binding = 7) uniform sampler2DArray splatLayers;

// Smooth value noise, 0..1, for the edges below.
float SplatHash(vec2 p)
{
	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float SplatNoise(vec2 p)
{
	vec2 i = floor(p), f = fract(p);
	vec2 u = f * f * (3.0 - 2.0 * f);
	return mix(mix(SplatHash(i), SplatHash(i + vec2(1.0, 0.0)), u.x),
		mix(SplatHash(i + vec2(0.0, 1.0)), SplatHash(i + vec2(1.0, 1.0)), u.x), u.y);
}

vec3 SplatAlbedo(vec3 base, vec2 uv)
{
	vec4 w = vec4(0.0, clamp(Tangent, 0.0, 1.0));
	w.x = clamp(1.0 - (w.y + w.z + w.w), 0.0, 1.0);

	// Wandering edges. The weights are the mesh's, interpolated across its
	// triangles, so left alone every boundary runs straight along them - a
	// staircase, seen from above. Each layer's weight is nudged by noise of
	// its own, a fraction of a texture repeat to a few repeats across, so the
	// edges wander the way ground does. Only where layers already meet:
	// ground that is all one thing stays so.
	vec4 nudge;
	nudge.x = SplatNoise(uv * 2.3) * 0.6 + SplatNoise(uv * 7.9 + 3.1) * 0.4;
	nudge.y = SplatNoise(uv * 2.3 + vec2(17.3, 5.1)) * 0.6 + SplatNoise(uv * 7.9 + vec2(9.4, 2.2)) * 0.4;
	nudge.z = SplatNoise(uv * 2.3 + vec2(-9.2, 13.7)) * 0.6 + SplatNoise(uv * 7.9 + vec2(-4.7, 8.8)) * 0.4;
	nudge.w = SplatNoise(uv * 2.3 + vec2(5.5, -21.4)) * 0.6 + SplatNoise(uv * 7.9 + vec2(12.6, -6.3)) * 0.4;
	vec4 meets = step(vec4(0.001), w) * step(0.001, 1.0 - max(max(w.x, w.y), max(w.z, w.w)));
	w = clamp(w + (nudge - 0.5) * 0.5 * meets, 0.0, 1.0);

	vec3 c1 = texture(splatLayers, vec3(uv, 0.0)).rgb;
	vec3 c2 = texture(splatLayers, vec3(uv, 1.0)).rgb;
	vec3 c3 = texture(splatLayers, vec3(uv, 2.0)).rgb;

	// Detail as height: brighter than its own average stands proud.
	const vec3 LUMA = vec3(0.299, 0.587, 0.114);
	vec4 mean = vec4(dot(textureLod(theTexture, uv, 16.0).rgb, LUMA),
		dot(textureLod(splatLayers, vec3(uv, 0.0), 16.0).rgb, LUMA),
		dot(textureLod(splatLayers, vec3(uv, 1.0), 16.0).rgb, LUMA),
		dot(textureLod(splatLayers, vec3(uv, 2.0), 16.0).rgb, LUMA));
	vec4 detail = vec4(dot(base, LUMA), dot(c1, LUMA), dot(c2, LUMA), dot(c3, LUMA)) - mean;
	vec4 present = step(vec4(0.001), w);
	vec4 s = w + detail * 0.8 * present;
	float top = max(max(s.x, s.y), max(s.z, s.w)) - 0.12;
	vec4 b = max(s - top, 0.0) * present;
	float sum = max(b.x + b.y + b.z + b.w, 1e-4);

	// The base layer wanders a little in brightness and warmth over a few
	// repeats of its texture, so a field does not show its tile.
	float wander = 0.5 + 0.25 * sin(uv.x * 0.61 + sin(uv.y * 0.47) * 2.0)
	                   + 0.25 * sin(uv.y * 0.83 + sin(uv.x * 0.37) * 2.0);
	base *= mix(vec3(0.86, 0.94, 0.90), vec3(1.14, 1.04, 0.76), wander);

	return (base * b.x + c1 * b.y + c2 * b.z + c3 * b.w) / sum;
}
#endif

// Storm lightning: a brief bright flood of light from the sky (world up = -Y).
// Light contributed by a lightning flash on a surface with normal N. Up-facing
// surfaces catch the most (the bolt lights the world from above); a flat fill
// keeps sides/undersides from staying black during the flash.
vec3 LightningLight(vec3 N)
{
    float up = clamp(-N.y, 0.0, 1.0);
    return lightningColor * lightningFlash * (0.35 + 0.65 * up);
}

// Toon / cel shading (Scene.toon): quantise the diffuse into flat bands + a hard
// specular. Overrides the lighting model when set (a global look).

// --- shadow mapping (directional sun; matrix + strength in the Scene block) ---
uniform sampler2D shadowMap;

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
#include "point_shadows.glsl"   // PointShadowSlot (cube shadows of the lamps)

// Finite-range falloff (reaches 0 at range): artist-friendly, no infinities
float Attenuate(float dist, float range)
{
	float a = clamp(1.0 - dist / range, 0.0, 1.0);
	return a * a;
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

// The tangent frame of a normal map, from screen-space derivatives solved
// exactly: T along +u, B up the texture (-v), so a map's +Y (green) means up,
// the OpenGL / glTF convention. Right for mirrored UVs and whichever way the
// projection flips the screen. (Until 2026-10-03 materials used Schuler's
// cotangent frame, whose adjugate form flips T and B when the screen mapping
// is mirrored, as the engine's Y-flipped projection always is: X came out
// inverted.)
mat3 UvFrame(vec3 N, vec3 p, vec2 uv)
{
	vec3 dp1 = dFdx(p);
	vec3 dp2 = dFdy(p);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);
	float s = (duv1.x * duv2.y - duv1.y * duv2.x < 0.0) ? -1.0 : 1.0;
	vec3 T = (dp1 * duv2.y - dp2 * duv1.y) * s;
	vec3 B = (dp2 * duv1.x - dp1 * duv2.x) * s;
	T -= N * dot(N, T);
	B -= N * dot(N, B);
	if (dot(T, T) < 1e-20 || dot(B, B) < 1e-20)
		return mat3(vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), N);   // no usable UVs: leave N as is
	return mat3(normalize(T), -normalize(B), N);
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
		// The bands are a stylistic choice made by eye: in a linear target,
		// decode them so they keep the levels they were picked at.
		float ramp = ToonRamp(NdotL);
		if (kinjoTargetLinear != 0)
			ramp = SrgbToLinear(vec3(ramp)).r;
		diffuseAccum += albedo * radiance * ramp;
		float sp = pow(max(dot(N, H), 0.0), max(matShininess, 1.0));
		specularAccum += radiance * step(0.5, sp) * matSpecular;   // hard highlight
	}
	else if (matLighting == 1)   // PBR (Cook-Torrance)
	{
		float NDF = DistributionGGX(N, H, surfRoughness);
		float G = GeometrySchlick(max(dot(N, V), 0.0), surfRoughness) *
		          GeometrySchlick(NdotL, surfRoughness);
		vec3 F = F0 + (1.0 - F0) * pow(1.0 - max(dot(H, V), 0.0), 5.0);
		vec3 spec = (NDF * G * F) / max(4.0 * max(dot(N, V), 0.0) * NdotL, 1e-4);
		vec3 kd = (vec3(1.0) - F) * (1.0 - surfMetallic);
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
#ifdef KINJO_SPLAT
	if ((matMaps & MAT_SPLAT) != 0)
		texColor.rgb = SplatAlbedo(texColor.rgb, uv);
#endif
	float alpha = texColor.a * matOpacity;
	if ((matMaps & MAT_GLTF) != 0)
	{
		// glTF alpha modes: blended surfaces blend; masked ones cut out below
		// the cutoff; otherwise (masked or opaque) alpha means nothing.
		if ((matMaps & MAT_ALPHA_BLEND) == 0)
		{
			if ((matMaps & MAT_ALPHA_MASK) != 0 && texColor.a < matAlphaCutoff)
				discard;
			alpha = 1.0;
		}
	}
	else if (texColor.a < 0.1)
		discard;

	surfMetallic = matMetallic;
	surfRoughness = matRoughness;
	vec4 metalRough = vec4(1.0);
	if ((matMaps & MAT_METAL_ROUGH_MAP) != 0)
	{
		metalRough = texture(metallicRoughnessMap, uv);
		surfRoughness *= metalRough.g;
		surfMetallic *= metalRough.b;
	}

	// Glass and other see-through surfaces keep the motion of what's behind them.
	oMotion = MotionVector(FragPos, PrevWorldPos, (matOpacity < 1.0) ? 0.0 : 1.0);

	vec3 albedo = texColor.rgb * matTint;

	// KHR_materials_unlit: the base colour as it is.
	if ((matMaps & MAT_UNLIT) != 0)
	{
		color = vec4(albedo, alpha);
		return;
	}

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
		vec3 skyRefl = (0.35 + sceneLit) * TargetColor(vec3(0.55, 0.70, 0.92));
		if (iblOn != 0)
			skyRefl = EnvReflection(reflect(-Vw, Nw), 0.04);   // the real sky (and its sun)
		vec3 wcol = mix(deep, skyRefl, fres);

		// Moving sun/lamp glints (sharp specular off the rippled normal).
		vec3 glint = vec3(0.0);
		if (dirLightDiffuse > 0.0)
		{
			vec3 H = normalize(Vw + normalize(-dirLightDir));
			glint += dirLightColor * pow(max(dot(Nw, H), 0.0), matShininess) * matSpecular;
		}
		int glintFirst, glintCount;
		LightRange(FragPos, glintFirst, glintCount);
		for (int k = glintFirst; k < glintFirst + glintCount; k++)
		{
			SceneLight l = GetLight(k);
			if (l.spot != 0) continue;   // point lights glint
			vec3 Lv = l.pos - FragPos;
			float dist = length(Lv);
			if (dist < 0.0001) continue;
			vec3 H = normalize(Vw + Lv / dist);
			glint += l.color * l.intensity * Attenuate(dist, l.range)
			         * pow(max(dot(Nw, H), 0.0), matShininess) * matSpecular;
		}

		float a = clamp(matOpacity + fres * 0.35 + max(glint.r, max(glint.g, glint.b)), 0.0, 1.0);
		color = vec4(wcol + glint, a);
		return;
	}

	// Base geometric normal, optionally perturbed by the normal map. The TBN
	// frame comes from screen-space derivatives (UvFrame), or with
	// matNormalMode == 1 takes its directions from the vertex tangents.
	vec3 N = normalize(Normal);
	if ((matMaps & MAT_DOUBLE_SIDED) != 0)
	{
		// A back face: light it with the normal of the side the camera sees.
		vec3 face = cross(dFdx(FragPos), dFdy(FragPos));
		if (dot(face, viewPos - FragPos) * dot(face, N) < 0.0)
			N = -N;
	}
	if (matHasNormal == 1)
	{
		vec3 mapN = texture(normalMap, uv).rgb * 2.0 - 1.0;
		if ((matMaps & MAT_NORMAL_XY) != 0)
			mapN.z = sqrt(max(1.0 - dot(mapN.xy, mapN.xy), 0.0));   // two-channel (BC5) map
		mapN.xy *= matNormalStrength;
		mat3 TBN = UvFrame(N, FragPos, uv);
		if ((matMaps & (MAT_GLTF | MAT_SPLAT)) == 0 && matNormalMode == 1 && dot(Tangent, Tangent) > 1e-12)
		{
			// Vertex tangents: smooth directions across faces, but their signs
			// (and the bitangent's) from the exact frame. The importer's
			// tangents follow its own UV convention, and nothing stores a
			// mirrored layout's handedness: taken as they were, the map's
			// up/down came out inverted.
			vec3 T = normalize(Tangent - N * dot(N, Tangent));  // Gram-Schmidt
			if (dot(T, TBN[0]) < 0.0)
				T = -T;
			vec3 B = cross(N, T);
			if (dot(B, TBN[1]) < 0.0)
				B = -B;
			TBN = mat3(T, B, N);
		}
		N = normalize(TBN * mapN);
	}

	vec3 V = normalize(viewPos - FragPos);

	vec3 diffuseAccum = vec3(0.0);
	vec3 specularAccum = vec3(0.0);
	vec3 F0 = mix(vec3(0.04), albedo, surfMetallic);   // PBR base reflectance

	// Directional fill (usually off in a point/spot-lit room)
	if (dirLightDiffuse > 0.0)
		AddLight(normalize(-dirLightDir), dirLightColor * dirLightDiffuse,
			N, V, albedo, F0, diffuseAccum, specularAccum);

	// Point and spot lights: the ones that reach this pixel's cluster
	// (lights.glsl), points first, then spots.
	int lightFirst, lightCount;
	LightRange(FragPos, lightFirst, lightCount);
	for (int k = lightFirst; k < lightFirst + lightCount; k++)
	{
		SceneLight l = GetLight(k);
		vec3 Lv = l.pos - FragPos;
		float dist = length(Lv);
		if (dist < 0.0001) continue;
		if (l.spot == 0)
		{
			vec3 radiance = l.color * l.intensity * Attenuate(dist, l.range);
			if (l.shadow >= 0)
				radiance *= PointShadowSlot(l.shadow, FragPos);
			AddLight(Lv / dist, radiance, N, V, albedo, F0, diffuseAccum, specularAccum);
		}
		else
		{
			// Distance falloff * cone falloff
			vec3 L = Lv / dist;
			float theta = dot(-L, normalize(l.dir));
			float cone = smoothstep(l.cosOuter, l.cosInner, theta);
			vec3 radiance = l.color * l.intensity * Attenuate(dist, l.range) * cone;
			AddLight(L, radiance, N, V, albedo, F0, diffuseAccum, specularAccum);
		}
	}

	// Sun shadow: darken the direct (diffuse+specular) contribution where the
	// fragment is occluded from the sun; ambient still fills shadowed areas.
	float ndotl = max(dot(N, normalize(-dirLightDir)), 0.0);
	// Cascaded maps when the engine renders them (Scene3DShadows.cpp), else the
	// single sun map. The cascades offset along the TRUE face normal (from
	// screen-space derivatives, turned toward the camera), not the vertex
	// normal: generated or smoothed normals - a box with no normals in its
	// file, stretched non-uniformly - can point almost anywhere, and offsetting
	// along one pushed the lookup inside the object.
	vec3 faceNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
	if (dot(faceNormal, viewPos - FragPos) < 0.0)
		faceNormal = -faceNormal;
	float shadow = (cascadeCount > 0)
		? 1.0 - shadowStrength * (1.0 - SunShadow(FragPos, faceNormal, normalize(dirLightDir)))
		: ShadowFactor(FragPos, ndotl);

	// Ambient: the scene's flat `ambient` colour - or, with image-based lighting
	// (a linear-workflow scene with a sky; environment.glsl), the sky's own
	// light arriving from the surface's direction, plus the sky reflected by
	// roughness and Fresnel. Cel shading keeps a flat ambient either way.
	// Ambient occlusion (ao.glsl) darkens this light only, never direct light.
	// Screen-space reflections (ao.glsl ScreenReflection) replace the sky's
	// reflection where they found something, and are the only reflection in a
	// room without a sky.
	// The material's own occlusion map (glTF) darkens ambient light as well.
	float materialOcclusion = 1.0;
	if ((matMaps & MAT_OCCLUSION_PACKED) != 0)
		materialOcclusion = mix(1.0, metalRough.r, matOcclusionStrength);
#ifdef KINJO_GL4
	else if ((matMaps & MAT_OCCLUSION_MAP) != 0)
		materialOcclusion = mix(1.0, texture(occlusionMap, uv).r, matOcclusionStrength);
#endif
	float ambientVisibility = AmbientVisibility(FragPos) * materialOcclusion;
	vec3 ambient = albedo * ambientColor * ambientVisibility;
	vec3 skySpecular = vec3(0.0);
	if (iblOn != 0 && toon == 1)
	{
		ambient = albedo * EnvDiffuseFlat() * ambientVisibility;
	}
	else if (toon == 0 && (iblOn != 0 || ssrOn != 0))
	{
		float NdotV = max(dot(N, V), 0.0);
		bool pbr = (matLighting == 1);
		// Blinn-Phong materials: a roughness from the highlight exponent, and
		// reflections scaled by their specular strength.
		float rough = pbr ? surfRoughness : sqrt(2.0 / (matShininess + 2.0));
		vec3 specF0 = pbr ? F0 : vec3(0.04);
		vec4 screen = ScreenReflection(FragPos);
		if (iblOn != 0)
		{
			vec3 fresnel = specF0 + (max(vec3(1.0 - rough), specF0) - specF0) * pow(1.0 - NdotV, 5.0);
			vec3 kd = pbr ? (vec3(1.0) - fresnel) * (1.0 - surfMetallic) : vec3(1.0);
			ambient = kd * albedo * EnvDiffuse(N) * ambientVisibility;
			vec2 brdf = EnvBrdf(NdotV, rough);
			vec3 reflected = EnvReflection(reflect(-V, N), rough);
			if (screen.a > 0.0)
				reflected = mix(reflected, screen.rgb, screen.a);
			skySpecular = reflected * (specF0 * brdf.x + brdf.y)
				* (pbr ? 1.0 : matSpecular) * SpecularVisibility(NdotV, ambientVisibility, rough);
		}
		else if (screen.a > 0.0)
		{
			// No sky maps (so no BRDF table): Karis' analytic fit of it.
			vec4 r = rough * vec4(-1.0, -0.0275, -0.572, 0.022) + vec4(1.0, 0.0425, 1.04, -0.04);
			float a004 = min(r.x * r.x, exp2(-9.28 * NdotV)) * r.x + r.y;
			vec2 brdf = vec2(-1.04, 1.04) * a004 + r.zw;
			skySpecular = screen.rgb * screen.a * (specF0 * brdf.x + brdf.y)
				* (pbr ? 1.0 : matSpecular) * SpecularVisibility(NdotV, ambientVisibility, rough);
		}
	}
	vec3 lit = ambient + skySpecular + (diffuseAccum + specularAccum) * shadow;

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

	vec3 emissive = matEmissive;
#ifdef KINJO_GL4
	if ((matMaps & MAT_EMISSIVE_MAP) != 0)
		emissive *= texture(emissiveMap, uv).rgb;
#endif
	lit += emissive;

	if (aoDebug != 0 && aoOn != 0)
		lit = vec3(ambientVisibility);   // KINJO_AO_DEBUG: the occlusion itself
	if (ClusterDebugView())
		lit = ClusterHeat(lightCount);   // KINJO_CLUSTER_DEBUG: how many lights reach here

	color = vec4(lit, alpha);
}
