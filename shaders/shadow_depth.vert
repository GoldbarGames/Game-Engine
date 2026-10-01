#version 330

// Shadow-map depth pass: render the scene from the sun's point of view. Only
// gl_Position (depth) matters; TexCoord is passed so the frag can alpha-cut
// sprite silhouettes. Character billboards supply an upright model matrix.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;

out vec2 TexCoord;

#include "shadow_pass.glsl"   // viewProj = light projection * light view
#include "draw.glsl"

struct DrawData { mat4 model; };
PER_DRAW(DrawData);

void main()
{
	gl_Position = viewProj * draw.model * vec4(pos, 1.0);
	TexCoord = tex;
}
