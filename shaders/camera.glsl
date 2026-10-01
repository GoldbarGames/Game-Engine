#ifndef KINJO_CAMERA_GLSL
#define KINJO_CAMERA_GLSL
// Shared camera matrices: the engine's "Camera" uniform block (std140, binding
// point 0 - see src/ENGINE/UniformBlocks.h). The renderer binds the right
// camera's buffer before every draw (Renderer::BindCameraBlock), so a shader
// only declares the block and uses `view` / `projection` directly.
layout(std140) uniform Camera
{
	mat4 view;
	mat4 projection;
};
#endif
