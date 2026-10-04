#ifndef KINJO_PANORAMA_GLSL
#define KINJO_PANORAMA_GLSL
// Where a world direction lands on a sky panorama (equirectangular: u = angle
// around the vertical axis, v = 0 at the zenith, 1 at the nadir), and back.
// Matches the skybox sphere in Skybox.cpp as drawn in a linear-workflow
// project (zenith up; the world is -Y up). Image-based lighting maps
// (render/Environment.cpp) are stored in this same layout.
const float PANO_PI = 3.14159265358979;

vec2 PanoramaUV(vec3 d)
{
	d = normalize(d);
	float u = atan(d.z, d.x) / (2.0 * PANO_PI);
	u = (u < 0.0) ? u + 1.0 : u;
	float v = acos(clamp(-d.y, -1.0, 1.0)) / PANO_PI;
	return vec2(u, v);
}

vec3 PanoramaDir(vec2 uv)
{
	float theta = uv.x * 2.0 * PANO_PI;
	float phi = uv.y * PANO_PI;
	return vec3(sin(phi) * cos(theta), -cos(phi), sin(phi) * sin(theta));
}
#endif
