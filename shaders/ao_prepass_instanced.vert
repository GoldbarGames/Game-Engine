#version 330

// Ambient-occlusion prepass, instanced props (the per-instance model matrix
// comes from vertex attributes, as in scene3d_instanced.vert). Pairs with
// ao_prepass.frag.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in mat4 instanceModel;   // occupies locations 3,4,5,6

out vec2 TexCoord;
out vec3 ViewPos;

#include "camera.glsl"

void main()
{
	vec4 viewPos = view * (instanceModel * vec4(pos, 1.0));
	gl_Position = projection * viewPos;
	TexCoord = tex;
	ViewPos = viewPos.xyz;
}
