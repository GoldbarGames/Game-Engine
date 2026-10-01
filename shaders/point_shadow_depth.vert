#version 330

// Omnidirectional point-light shadow depth pass: one of 6 cube faces per draw.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;
out vec3 WorldPos;

#include "shadow_pass.glsl"   // viewProj = face projection * face view
#include "draw.glsl"

struct DrawData { mat4 model; };
PER_DRAW(DrawData);

void main()
{
	vec4 w = draw.model * vec4(pos, 1.0);
	WorldPos = w.xyz;
	gl_Position = viewProj * w;
	TexCoord = tex;
}
