#ifndef KINJO_SCENE_GLSL
#define KINJO_SCENE_GLSL
// Frame-level scene environment: the engine's "Scene" uniform block (std140,
// binding point 1 - see src/ENGINE/UniformBlocks.h). Filled by
// Scene3D::ApplyLighting and re-uploaded only when its contents change.
//
// Member names match the loose uniforms these replaced, so shader bodies read
// them unchanged. The C++ mirror is SceneBlockData in Scene3D.cpp; keep the two
// in the same order (the engine checks the offsets at startup and logs any
// mismatch). Comments give the std140 byte offset.

const int MAX_POINTS = 8;
const int MAX_SPOTS = 4;
const int MAX_PT_SHADOW_SLOTS = 8;   // block capacity; see MAX_PT_SHADOWS for samplers

layout(std140) uniform Scene
{
	vec3  ambientColor;       //    0  global fill so unlit areas aren't pure black
	float lightningFlash;     //   12  storm flash, 0..~1 during a strike
	vec3  dirLightDir;        //   16  directional fill (diffuse 0 = off)
	float dirLightDiffuse;    //   28
	vec3  dirLightColor;      //   32
	float shadowStrength;     //   44  how much shadowed areas darken (0..1)
	vec3  lightningColor;     //   48
	float uTime;              //   60  seconds since start (animated materials)
	vec3  viewPos;            //   64  camera world position
	int   toon;               //   76  cel shading on/off (a global look)
	mat4  lightSpaceMatrix;   //   80  sun shadow map projection * view
	int   shadowsOn;          //  144
	int   pointCount;         //  148
	int   spotCount;          //  152
	int   pointShadowCount;   //  156

	vec3  pointPos[MAX_POINTS];          //  160  (std140 arrays: 16-byte stride)
	vec3  pointColor[MAX_POINTS];        //  288
	float pointRange[MAX_POINTS];        //  416
	float pointIntensity[MAX_POINTS];    //  544

	vec3  spotPos[MAX_SPOTS];            //  672
	vec3  spotDir[MAX_SPOTS];            //  736
	vec3  spotColor[MAX_SPOTS];          //  800
	float spotRange[MAX_SPOTS];          //  864
	float spotIntensity[MAX_SPOTS];      //  928
	float spotCosInner[MAX_SPOTS];       //  992
	float spotCosOuter[MAX_SPOTS];       // 1056

	// Point-light (cube) shadow casters: position, far plane, and the packed
	// point-light index each one shadows.
	vec3  pointShadowPositions[MAX_PT_SHADOW_SLOTS];  // 1120
	float pointShadowFars[MAX_PT_SHADOW_SLOTS];       // 1248
	int   pointShadowLightIdx[MAX_PT_SHADOW_SLOTS];   // 1376
};                                                    // 1504 bytes
#endif
