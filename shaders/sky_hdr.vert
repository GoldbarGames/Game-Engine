#version 330

// A sky the game painted itself (Scene3D::SetSkyImage, src/ENGINE/SkyImage.cpp):
// the sky sphere round the camera at the sky's radius, as sky.vert, with the
// per-draw data of sky_hdr.frag (the sun's disc too).
layout (location = 0) in vec3 pos;   // the unit sky sphere (Skybox::meshSkySphere)

out vec3 Dir;   // world direction (up is -Y)

#include "camera.glsl"
#include "draw.glsl"

struct DrawData   // as sky_hdr.frag's
{
	vec4 p0;     // a multiplier rgb (1), -
	vec4 p1;     // the sky's radius, -, -, -
	vec4 sun0;   // toward the sun xyz, sin(its angular radius)
	vec4 sun1;   // the disc's radiance rgb (0: none), flatten
	vec4 sun2;   // its lower edge's tint rgb, -
	vec4 sun3;   // its upper edge's tint rgb, -
};
PER_DRAW(DrawData);

void main()
{
	// The camera from the view matrix (rotation and translation only).
	vec3 cam = -transpose(mat3(view)) * view[3].xyz;
	Dir = normalize(pos);
	gl_Position = projection * view * vec4(cam + Dir * draw.p1.x, 1.0);
}
