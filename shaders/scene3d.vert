#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;
layout (location = 7) in vec3 tangent;   // from Model.cpp (CalcTangentSpace)

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 PrevWorldPos;   // where this point was last frame (motion vectors, motion.glsl)

#include "camera.glsl"
#include "draw.glsl"

// Per draw: the model matrix, transpose(inverse(mat3(model))) to keep
// normals correct under non-uniform scale, and how far the model moved since
// last frame (for motion vectors; 0 when still). 128 bytes, the push-constant
// budget.
struct DrawData
{
	mat4 model;
	mat3 normalMatrix;
	vec3 motionOffset;
};
PER_DRAW(DrawData);

// Animated water (matLighting == 2): large low-frequency swells are displaced
// into the actual geometry here so waves show in silhouette against the far
// bank; the fragment shader adds finer ripples on top. Both share uTime
// (Scene block); matLighting and the per-surface water tuning are in the
// Material block.
#include "scene.glsl"
#include "material.glsl"

void main()
{
	vec4 worldPos = draw.model * vec4(pos, 1.0);
	vec3 N = draw.normalMatrix * normal;

	if (matLighting == 2)
	{
		// Three big directional swells (bigger/slower than the fragment ripples).
		vec4 W[3] = vec4[3](              // dir.x, dir.z, freq, speed
			vec4( 1.0,  0.35, 0.0038, 0.9),
			vec4(-0.6,  1.00, 0.0052, 1.2),
			vec4( 0.8, -0.70, 0.0071, 1.5));
		// Relative heights, scaled so their sum == uWaterAmp.
		float A[3] = float[3](uWaterAmp * 0.5, uWaterAmp * 0.333, uWaterAmp * 0.167);

		vec2 p = worldPos.xz;
		float h = 0.0, hx = 0.0, hz = 0.0;      // height + analytic XZ slope
		for (int i = 0; i < 3; i++)
		{
			vec2 d = normalize(W[i].xy);
			float f = W[i].z * uWaterWaveScale;
			float ph = (d.x * p.x + d.y * p.y) * f + uTime * W[i].w;
			h += A[i] * sin(ph);
			float c = A[i] * f * cos(ph);
			hx += d.x * c;
			hz += d.y * c;
		}

		// Shoreline taper: fade the waves to zero toward the mesh edge so the
		// waterline meets the land flush. pos is local (water_blob.obj radius
		// ~390u); the fade band widens as uWaterShoreFade -> 1.
		if (uWaterShoreFade > 0.001)
		{
			float edgeR = 390.0;
			float fadeStart = edgeR * (1.0 - uWaterShoreFade);
			float taper = 1.0 - smoothstep(fadeStart, edgeR, length(pos.xz));
			h *= taper; hx *= taper; hz *= taper;
		}

		worldPos.y += h;                        // displace the surface up/down
		N = normalize(vec3(hx, -1.0, hz));      // slope normal (flat => -Y up)
	}

	FragPos = worldPos.xyz;
	PrevWorldPos = worldPos.xyz - draw.motionOffset;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
	Normal = N;
	// A splat material's tangent slot carries its ground layers' weights,
	// which are not a direction and must not be turned with the model.
	Tangent = ((matMaps & MAT_SPLAT) != 0) ? tangent : draw.normalMatrix * tangent;
}
