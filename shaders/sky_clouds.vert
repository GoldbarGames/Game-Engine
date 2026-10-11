#version 330

// A layer of cloud over the sky's panorama (src/ENGINE/SkyClouds.h): the sky
// sphere again, round the camera at the sky's radius. The fragment shader
// follows each pixel's direction up to the layer.
layout (location = 0) in vec3 pos;   // the unit sky sphere (Skybox::meshSkySphere)

out vec3 Dir;          // world direction (up is -Y)
flat out vec3 CamPos;  // the camera, world

#include "camera.glsl"
#include "draw.glsl"

// One layer (SkyClouds.cpp sets them; sky_clouds.frag reads them).
struct DrawData
{
	vec4 p0;   // heaped (0/1), height, size, cover
	vec4 p1;   // soft, warp, opacity, depth
	vec4 p2;   // stretch x, stretch y, angle, haze distance
	vec4 p3;   // lit rgb, silver
	vec4 p4;   // shade rgb, thick dark
	vec4 p5;   // drift x, drift z, base dark, noise seed + stagger turn
	vec4 p6;   // toward the sun xyz, the sky's radius
	vec4 p7;   // toward the rim light xyz, glare
};
PER_DRAW(DrawData);

void main()
{
	// The camera from the view matrix (rotation and translation only).
	vec3 cam = -transpose(mat3(view)) * view[3].xyz;
	Dir = normalize(pos);
	CamPos = cam;
	gl_Position = projection * view * vec4(cam + Dir * draw.p6.w, 1.0);
}
