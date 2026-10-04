#version 330

// Ambient-occlusion prepass (Phase 1.5 item 6; src/ENGINE/render/AmbientOcclusion.h):
// the opaque scene models again, writing linear view depth and view-space
// normals for the occlusion pass. Same transform as scene3d.vert.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;
out vec3 ViewPos;

#include "camera.glsl"
#include "draw.glsl"

struct DrawData
{
	mat4 model;
};
PER_DRAW(DrawData);

void main()
{
	vec4 viewPos = view * (draw.model * vec4(pos, 1.0));
	gl_Position = projection * viewPos;
	TexCoord = tex;
	ViewPos = viewPos.xyz;
}
