#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 PrevWorldPos;   // where this point was last frame (motion vectors, motion.glsl)

#include "camera.glsl"
#include "draw.glsl"

// The upright billboard facing the camera, and how far the character moved
// since last frame (for motion vectors; 0 when still).
struct DrawData
{
	mat4 model;
	vec3 motionOffset;
};
PER_DRAW(DrawData);

void main()
{
	vec4 worldPos = draw.model * vec4(pos, 1.0);
	FragPos = worldPos.xyz;
	PrevWorldPos = worldPos.xyz - draw.motionOffset;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
}
