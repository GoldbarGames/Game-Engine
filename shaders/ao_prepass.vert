#version 330

// Ambient-occlusion prepass (Phase 1.5 item 6; src/ENGINE/render/AmbientOcclusion.h):
// the opaque scene models again, writing linear view depth and view-space
// normals for the occlusion pass. Same transform as scene3d.vert, the wind's
// sway included (wind.glsl), so a swaying crown keeps its occlusion.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 7) in vec3 tangent;   // a `wind` mesh's sway weights

out vec2 TexCoord;
out vec3 ViewPos;

#include "camera.glsl"
#include "draw.glsl"
#include "scene.glsl"      // the wind (windParams)
#include "material.glsl"   // matWind
#include "wind.glsl"

struct DrawData
{
	mat4 model;
};
PER_DRAW(DrawData);

void main()
{
	vec4 worldPos = draw.model * vec4(pos, 1.0);
	if (matWind != 0.0)
		worldPos.xyz += WindSway(worldPos.xyz, WindWeights(pos, tangent, draw.model), true);
	vec4 viewPos = view * worldPos;
	gl_Position = projection * viewPos;
	TexCoord = tex;
	ViewPos = viewPos.xyz;
}
