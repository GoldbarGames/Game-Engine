#version 330

// Ambient-occlusion prepass, instanced props (the per-instance model matrix
// comes from vertex attributes, as in scene3d_instanced.vert), swaying in the
// wind as that does (wind.glsl). Pairs with ao_prepass.frag.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in mat4 instanceModel;   // occupies locations 3,4,5,6
layout (location = 7) in vec3 tangent;         // a `wind` mesh's sway weights

out vec2 TexCoord;
out vec3 ViewPos;

#include "camera.glsl"
#include "scene.glsl"      // the wind (windParams)
#include "material.glsl"   // matWind
#include "wind.glsl"

void main()
{
	vec4 worldPos = instanceModel * vec4(pos, 1.0);
	if (matWind != 0.0)
		worldPos.xyz += WindSway(worldPos.xyz, WindWeights(pos, tangent, instanceModel), true);
	vec4 viewPos = view * worldPos;
	gl_Position = projection * viewPos;
	TexCoord = tex;
	ViewPos = viewPos.xyz;
}
