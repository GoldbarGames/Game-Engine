#version 430

// GPU-driven variant of scene3d.vert (Phase 1.5 item 11): the model matrix
// comes from the frame's instance list (scene_instances.glsl), picked by a
// per-instance index that GPU culling wrote (cull_instances.comp). Pairs with
// scene3d.frag (same varyings). Opaque, non-water models only.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;
layout (location = 3) in uint instanceIndex;
layout (location = 7) in vec3 tangent;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 PrevWorldPos;   // where this point was last frame (motion vectors, motion.glsl)

#include "camera.glsl"
#include "scene_instances.glsl"

// How far each instance moved since last frame (0 when still, or when motion
// vectors aren't being written).
layout(std430, binding = 1) readonly buffer SceneMotion
{
	vec4 sceneMotion[];
};

void main()
{
	mat4 model = sceneInstances[instanceIndex].model;
	vec4 worldPos = model * vec4(pos, 1.0);
	// Per-instance normal matrix (transpose(inverse) corrects non-uniform scale).
	mat3 nm = transpose(inverse(mat3(model)));

	FragPos = worldPos.xyz;
	PrevWorldPos = worldPos.xyz - sceneMotion[instanceIndex].xyz;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
	Normal = nm * normal;
	Tangent = nm * tangent;
}
