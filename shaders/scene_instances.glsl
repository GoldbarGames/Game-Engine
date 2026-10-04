#ifndef KINJO_SCENE_INSTANCES_GLSL
#define KINJO_SCENE_INSTANCES_GLSL
// GPU-driven Scene3D models (Phase 1.5 item 11, src/ENGINE/Scene3DGpuDriven.cpp):
// the frame's instance list, one record per drawn model, in a storage buffer
// (std430, binding 0). The C++ mirror is InstanceRecord there. GL 4.3+ only.
struct SceneInstance
{
	mat4  model;
	vec4  sphere;   // world-space bounding sphere: xyz centre, w radius (< 0 = unknown: never culled)
	uvec4 info;     // x = batch, y = flags (1 = drawn in the colour passes, 2 = casts shadows)
};

layout(std430, binding = 0) readonly buffer SceneInstances
{
	SceneInstance sceneInstances[];
};
#endif
