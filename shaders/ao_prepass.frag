#version 330

// Ambient-occlusion prepass: linear view depth (target 0, R32F) and the
// view-space normal (target 1). Cut out where the albedo is transparent,
// exactly as scene3d.frag discards, so both passes see the same surfaces.
// Writes data, not colour: KINJO_LINEAR_OUTPUT keeps the linear workflow
// from adapting it.
#define KINJO_LINEAR_OUTPUT

in vec2 TexCoord;
in vec3 ViewPos;

layout (location = 0) out vec4 outDepth;
layout (location = 1) out vec4 outNormal;

uniform sampler2D theTexture;
uniform sampler2D metallicRoughnessMap;   // an imported (glTF) material's, unit 2
#include "material.glsl"   // matUVTile, matMaps

void main()
{
	vec2 uv = TexCoord * matUVTile;
	float a = texture(theTexture, uv).a;
	if ((matMaps & MAT_GLTF) != 0)
	{
		// glTF: only masked surfaces cut out (blended ones aren't drawn here).
		if ((matMaps & MAT_ALPHA_MASK) != 0 && a < matAlphaCutoff)
			discard;
	}
	else if (a < (((matMaps & MAT_ALPHA_MASK) != 0) ? matAlphaCutoff : 0.1))
		discard;   // (a materials.txt `cutout` sets the mask bit and its threshold)

	// The TRUE face normal (from screen-space derivatives, turned toward the
	// camera), not the mesh's: the occlusion search measures horizons in the
	// depth this pass writes, so its normal has to agree with that depth. A
	// mesh's smoothed or authored normals often don't (a box with averaged
	// corner normals, stretched by scaleaxis), and a normal tilted off its
	// surface makes the surface occlude itself.
	vec3 N = normalize(cross(dFdx(ViewPos), dFdy(ViewPos)));
	if (dot(N, ViewPos) > 0.0)
		N = -N;

	// Alpha: the surface's roughness, for screen-space reflections (1 = don't
	// reflect: rough, or a Blinn-Phong material without specular).
	float rough = (matLighting == 1) ? matRoughness : sqrt(2.0 / (matShininess + 2.0));
	if ((matMaps & MAT_METAL_ROUGH_MAP) != 0)
		rough *= texture(metallicRoughnessMap, uv).g;
	if (matLighting != 1 && matSpecular <= 0.001)
		rough = 1.0;

	outDepth = vec4(-ViewPos.z, 0.0, 0.0, 1.0);
	outNormal = vec4(N * 0.5 + 0.5, clamp(rough, 0.0, 1.0));
}
