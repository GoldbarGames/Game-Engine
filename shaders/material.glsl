#ifndef KINJO_MATERIAL_GLSL
#define KINJO_MATERIAL_GLSL
// Surface material: the engine's "Material" uniform block (std140, binding
// point 2 - see src/ENGINE/UniformBlocks.h). Filled by Scene3D::ApplyMaterial;
// buffers are cached by content, so each distinct material (and each lake's
// water tuning) uploads once.
//
// Member names match the loose uniforms these replaced. The C++ mirror is
// MaterialBlockData in Scene3D.cpp (checked against these offsets at startup).

layout(std140) uniform Material
{
	vec3  matTint;            //  0
	float matFresnel;         // 12
	vec3  matEmissive;        // 16
	float matNormalStrength;  // 28
	vec2  matUVTile;          // 32
	float matSpecular;        // 40  Phong
	float matShininess;       // 44
	float matMetallic;        // 48  PBR
	float matRoughness;       // 52
	float matOpacity;         // 56
	int   matHasNormal;       // 60
	int   matNormalMode;      // 64  0 = screen-space TBN, 1 = vertex tangents
	int   matLighting;        // 68  0 = Blinn-Phong, 1 = PBR, 2 = water
	// Water tuning (matLighting == 2): vertex swells + fragment ripples.
	float uWaterAmp;          // 72  total wave height (world units)
	float uWaterWaveScale;    // 76  spatial frequency multiplier
	float uWaterShoreFade;    // 80  0 = waves reach the edge; 1 = calm shore
	float uWaterChoppy;       // 84  surface-ripple strength (1.0 = default)
	// Maps and modes of a material imported with a model (glTF 2.0,
	// src/ENGINE/ModelMaterials.h). 0 for materials.txt materials, apart from
	// MAT_NORMAL_XY.
	int   matMaps;            // 88  MAT_* bits below
	float matAlphaCutoff;     // 92  MAT_ALPHA_MASK: alpha below this is cut out
	float matOcclusionStrength; // 96 how far the occlusion map darkens ambient light
	// Swaying in the wind (`wind <sway> [height]`, wind.glsl): how far the top
	// leans in a fresh breeze, world units (0 = still); and, if over 0, the
	// height the sway is weighted by (else the mesh's tangent slot weights it).
	float matWind;            // 100
	float matWindHeight;      // 104
	// The flutter's share (`wind <sway> <height> <flutter>`; 1 unless given):
	// a tree's bark 0, its leaves 1.
	float matFlutter;         // 108
	// Sunlight through thin leaves from behind (`translucency <v>`; 0 = none).
	float matTranslucency;    // 112
	// MATF_* bits below.
	int   matFlags;           // 116
};                            // 128 bytes (rounded to vec4)

const int MATF_OCCLUDER_FADE = 1;      // `fade on`: dissolves near the occluder-fade line (scene.glsl)

const int MAT_METAL_ROUGH_MAP  = 1;    // metallicRoughnessMap (unit 2): G = roughness, B = metallic
const int MAT_OCCLUSION_PACKED = 2;    // ... and R = occlusion
const int MAT_OCCLUSION_MAP    = 4;    // occlusionMap (unit 5), R (KINJO_GL4 only)
const int MAT_EMISSIVE_MAP     = 8;    // emissiveMap (unit 6), times matEmissive (KINJO_GL4 only)
const int MAT_NORMAL_XY        = 16;   // the normal map holds x and y only (BC5): rebuild z
const int MAT_GLTF             = 32;   // glTF conventions: normal map +Y = up the texture; alpha by mode
const int MAT_DOUBLE_SIDED     = 64;   // back faces are lit as front faces
const int MAT_ALPHA_MASK       = 128;  // cut out below matAlphaCutoff, else opaque
const int MAT_ALPHA_BLEND      = 256;  // blended (drawn in the transparent pass)
const int MAT_UNLIT            = 512;  // KHR_materials_unlit: the base colour as it is
const int MAT_SPLAT            = 1024; // splatLayers (unit 7): three ground layers over the
                                       // albedo, weighted per vertex by the tangent slot
                                       // (GLSL 4.20+ only; scene3d.frag)
#endif
