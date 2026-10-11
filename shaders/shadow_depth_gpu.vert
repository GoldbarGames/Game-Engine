#version 430

// Sun shadow-map depth pass, GPU-driven models (Phase 1.5 item 11): as
// shadow_depth.vert, with the model matrix from the frame's instance list.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in uint instanceIndex;   // low 24 bits (the top 8: a level fade, colour passes only)

out vec2 TexCoord;

#include "shadow_pass.glsl"   // viewProj = light projection * light view
#include "scene_instances.glsl"

void main()
{
	gl_Position = viewProj * sceneInstances[instanceIndex & 0x00FFFFFFu].model * vec4(pos, 1.0);
	TexCoord = tex;
}
