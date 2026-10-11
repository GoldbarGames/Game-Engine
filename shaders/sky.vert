#version 330

// The sky's panorama with the sun and the moon (src/ENGINE/SkyBodies.h): the sky
// sphere round the camera at the sky's radius. The fragment shader looks the
// panorama up by direction.
layout (location = 0) in vec3 pos;   // the unit sky sphere (Skybox::meshSkySphere)

out vec3 Dir;   // world direction (up is -Y)

#include "camera.glsl"
#include "draw.glsl"

struct DrawData
{
	vec4 p0;   // the panorama's tint rgb, cross-fade toward the next (0..1)
	vec4 p1;   // the sky's radius, has a moon face (0/1), the face's width in texels, -
};
PER_DRAW(DrawData);

void main()
{
	// The camera from the view matrix (rotation and translation only).
	vec3 cam = -transpose(mat3(view)) * view[3].xyz;
	Dir = normalize(pos);
	gl_Position = projection * view * vec4(cam + Dir * draw.p1.x, 1.0);
}
