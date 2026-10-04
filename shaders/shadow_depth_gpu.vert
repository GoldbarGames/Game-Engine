#version 430

// Sun shadow-map depth pass, GPU-driven models (Phase 1.5 item 11): as
// shadow_depth.vert, with the model matrix from the frame's instance list.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in uint instanceIndex;

out vec2 TexCoord;

#include "shadow_pass.glsl"   // viewProj = light projection * light view
#include "scene_instances.glsl"

void main()
{
	gl_Position = viewProj * sceneInstances[instanceIndex].model * vec4(pos, 1.0);
	TexCoord = tex;
}
