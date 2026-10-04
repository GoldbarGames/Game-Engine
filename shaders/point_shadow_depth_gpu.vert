#version 430

// Point-light cube shadow depth pass, GPU-driven models (Phase 1.5 item 11):
// as point_shadow_depth.vert, with the model matrix from the frame's instance list.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in uint instanceIndex;

out vec2 TexCoord;
out vec3 WorldPos;

#include "shadow_pass.glsl"   // viewProj = face projection * face view
#include "scene_instances.glsl"

void main()
{
	vec4 w = sceneInstances[instanceIndex].model * vec4(pos, 1.0);
	WorldPos = w.xyz;
	gl_Position = viewProj * w;
	TexCoord = tex;
}
