#version 430

// Ambient-occlusion prepass, GPU-driven models (Phase 1.5 item 11): the model
// matrix comes from the frame's instance list, as in scene3d_gpu.vert. Pairs
// with ao_prepass.frag.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in uint instanceIndex;

out vec2 TexCoord;
out vec3 ViewPos;

#include "camera.glsl"
#include "scene_instances.glsl"

void main()
{
	vec4 viewPos = view * (sceneInstances[instanceIndex].model * vec4(pos, 1.0));
	gl_Position = projection * viewPos;
	TexCoord = tex;
	ViewPos = viewPos.xyz;
}
