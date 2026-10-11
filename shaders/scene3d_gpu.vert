#version 430

// GPU-driven variant of scene3d.vert (Phase 1.5 item 11): the model matrix
// comes from the frame's instance list (scene_instances.glsl), picked by a
// per-instance index that GPU culling wrote (cull_instances.comp). Pairs with
// scene3d.frag (same varyings). Opaque, non-water models only. Sways in the
// wind as scene3d.vert does (since 2026-10-10: before, swaying models stayed
// on the CPU path).
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;
layout (location = 3) in uint instanceIndex;   // low 24 bits; the top 8 a level-of-detail fade
layout (location = 7) in vec3 tangent;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 PrevWorldPos;   // where this point was last frame (motion vectors, motion.glsl)
out vec3 RestPos;        // where it stands out of the wind: shadows are looked up here
flat out vec4 ModelBase; // the model's origin, and its level-of-detail fade (scene3d.frag)

#include "camera.glsl"
#include "scene_instances.glsl"
#include "scene.glsl"      // the wind (windParams)
#include "material.glsl"   // matWind
#include "wind.glsl"

// How far each instance moved since last frame (0 when still, or when motion
// vectors aren't being written).
layout(std430, binding = 1) readonly buffer SceneMotion
{
	vec4 sceneMotion[];
};

void main()
{
	uint index = instanceIndex & 0x00FFFFFFu;
	mat4 model = sceneInstances[index].model;
	vec4 worldPos = model * vec4(pos, 1.0);
	// Per-instance normal matrix (transpose(inverse) corrects non-uniform scale).
	mat3 nm = transpose(inverse(mat3(model)));
	vec3 moved = sceneMotion[index].xyz;

	// Swaying in the wind (wind.glsl): last frame's sway goes into the motion
	// vector, and shadows are looked up at rest.
	RestPos = worldPos.xyz;
	if (matWind != 0.0)
	{
		vec3 w = WindWeights(pos, tangent, model);
		worldPos.xyz += WindSway(RestPos, w, true);
		PrevWorldPos = RestPos + WindSway(RestPos, w, false) - moved;
	}
	else
		PrevWorldPos = worldPos.xyz - moved;

	FragPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
	Normal = nm * normal;
	// A splat material's tangent slot carries its ground layers' weights,
	// which are not a direction and must not be turned with the model (as
	// scene3d.vert; TrainRails' terrain, since 2026-10-10 on this path).
	Tangent = ((matMaps & MAT_SPLAT) != 0) ? tangent : nm * tangent;
	ModelBase = vec4(model[3].xyz, float(instanceIndex >> 24u));
}
