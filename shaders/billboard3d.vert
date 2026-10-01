#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;
out vec3 FragPos;

#include "camera.glsl"
#include "draw.glsl"

struct DrawData { mat4 model; };   // upright billboard facing the camera
PER_DRAW(DrawData);

void main()
{
	vec4 worldPos = draw.model * vec4(pos, 1.0);
	FragPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
}
