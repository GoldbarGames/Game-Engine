#version 330

// Instanced variant of scene3d.vert: the per-instance model matrix comes from
// vertex attributes (locations 3-6, uploaded by Mesh::SetInstances) instead of a
// uniform, so many copies of one prop draw in a single glDrawElementsInstanced.
// Pairs with scene3d.frag (same varyings). Opaque props only - no water branch.
// Sways in the wind as scene3d.vert does (since 2026-10-10).
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;
layout (location = 3) in mat4 instanceModel;   // occupies locations 3,4,5,6
layout (location = 7) in vec3 tangent;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 PrevWorldPos;   // instanced props don't move: last frame's = this frame's (but for the wind)
out vec3 RestPos;        // where it stands out of the wind: shadows are looked up here
flat out vec4 ModelBase; // the model's origin (the occluder fade); w 0: no level fade on this path

#include "camera.glsl"
#include "scene.glsl"      // the wind (windParams)
#include "material.glsl"   // matWind
#include "wind.glsl"

void main()
{
	vec4 worldPos = instanceModel * vec4(pos, 1.0);
	// Per-instance normal matrix (transpose(inverse) corrects non-uniform scale).
	mat3 nm = transpose(inverse(mat3(instanceModel)));

	RestPos = worldPos.xyz;
	if (matWind != 0.0)
	{
		vec3 w = WindWeights(pos, tangent, instanceModel);
		worldPos.xyz += WindSway(RestPos, w, true);
		PrevWorldPos = RestPos + WindSway(RestPos, w, false);
	}
	else
		PrevWorldPos = worldPos.xyz;

	FragPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
	Normal = nm * normal;
	// A splat material's tangent slot carries ground layer weights: not turned
	// (as scene3d.vert).
	Tangent = ((matMaps & MAT_SPLAT) != 0) ? tangent : nm * tangent;
	ModelBase = vec4(instanceModel[3].xyz, 0.0);
}
