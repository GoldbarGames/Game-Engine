#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

// Per-instance attributes
layout (location = 3) in mat4 instanceMatrix;  // locations 3-6
layout (location = 7) in vec4 texData;         // xy = texOffset, zw = texFrame
layout (location = 8) in vec4 instanceColor;

out vec4 vertexColor;
out vec2 TexCoord;
out vec4 spriteColorOut;

uniform mat4 projection;
uniform mat4 view;

void main()
{
	gl_Position = projection * view * instanceMatrix * vec4(pos.x, pos.y, pos.z, 1.0);
	vertexColor = vec4(clamp(pos, 0.0f, 1.0f), 1.0f);

	vec2 texOffset = texData.xy;
	vec2 texFrame = texData.zw;
	TexCoord = texOffset + (texFrame * tex);

	spriteColorOut = instanceColor;
}
