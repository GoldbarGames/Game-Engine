#version 330

// Instanced variant of scene3d.vert: the per-instance model matrix comes from
// vertex attributes (locations 3-6, uploaded by Mesh::SetInstances) instead of a
// uniform, so many copies of one prop draw in a single glDrawElementsInstanced.
// Pairs with scene3d.frag (same varyings). Opaque props only - no water branch.
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;
layout (location = 3) in mat4 instanceModel;   // occupies locations 3,4,5,6
layout (location = 7) in vec3 tangent;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 PrevWorldPos;   // instanced props don't move: last frame's = this frame's

#include "camera.glsl"

void main()
{
	vec4 worldPos = instanceModel * vec4(pos, 1.0);
	// Per-instance normal matrix (transpose(inverse) corrects non-uniform scale).
	mat3 nm = transpose(inverse(mat3(instanceModel)));

	FragPos = worldPos.xyz;
	PrevWorldPos = worldPos.xyz;
	gl_Position = projection * view * worldPos;
	TexCoord = tex;
	Normal = nm * normal;
	Tangent = nm * tangent;
}
