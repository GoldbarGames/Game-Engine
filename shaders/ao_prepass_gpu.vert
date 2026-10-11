#version 430

// Ambient-occlusion prepass, GPU-driven models (Phase 1.5 item 11): the model
// matrix comes from the frame's instance list, as in scene3d_gpu.vert, and it
// sways in the wind as that does (wind.glsl). Pairs with ao_prepass.frag.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 3) in uint instanceIndex;   // low 24 bits (the top 8: a level fade)
layout (location = 7) in vec3 tangent;         // a `wind` mesh's sway weights

out vec2 TexCoord;
out vec3 ViewPos;

#include "camera.glsl"
#include "scene_instances.glsl"
#include "scene.glsl"      // the wind (windParams)
#include "material.glsl"   // matWind
#include "wind.glsl"

void main()
{
	mat4 model = sceneInstances[instanceIndex & 0x00FFFFFFu].model;
	vec4 worldPos = model * vec4(pos, 1.0);
	if (matWind != 0.0)
		worldPos.xyz += WindSway(worldPos.xyz, WindWeights(pos, tangent, model), true);
	vec4 viewPos = view * worldPos;
	gl_Position = projection * viewPos;
	TexCoord = tex;
	ViewPos = viewPos.xyz;
}
