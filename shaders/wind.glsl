#ifndef KINJO_WIND_GLSL
#define KINJO_WIND_GLSL
// THE WIND'S PUSH ON A PLANT (src/ENGINE/Wind.h). Include scene.glsl
// (windParams) and material.glsl (matWind, matWindHeight, matFlutter) first.
//
// A plant leans downwind, more in each gust - and the gusts travel across the
// country with the wind, so a field of grass ripples and one tree moves after
// its neighbour - and it rocks about that lean at its own pace. Leaves and
// blade tips also flutter, quickly and a little, each on its own.

const float WIND_TAU = 6.2831853;

// A rate of k whole turns in the clock's 4096 s wrap (Wind.cpp), so the wrap
// is never seen.
float WindRate(float k) { return WIND_TAU * k / 4096.0; }

// How much each vertex moves: x = bend (0 root .. 1 top), y = flutter, z =
// phase 0..1. With a material height, from the vertex's own height in the
// model (visual up is local -Y), squared, and a phase from where the model
// stands; else from the mesh's tangent slot, which a mesh built for it fills.
vec3 WindWeights(vec3 localPos, vec3 tangentSlot, mat4 model)
{
	if (matWindHeight > 0.0)
	{
		float k = clamp(-localPos.y / matWindHeight, 0.0, 1.0);
		float phase = fract(sin(dot(model[3].xz, vec2(12.9898, 78.233))) * 43758.5453);
		return vec3(k * k, k, phase);
	}
	return tangentSlot;
}

// How far a vertex standing at `restPos` (world) is moved: this frame, or
// last frame (`now` false, for motion vectors).
vec3 WindSway(vec3 restPos, vec3 w, bool now)
{
	float strength = windParams.z;
	if (strength <= 0.0 || matWind == 0.0 || (w.x <= 0.0 && w.y <= 0.0))
		return vec3(0.0);
	vec2 dir = windParams.xy;
	float t = now ? windParams.w : windParams2.x;
	float travel = now ? windParams2.z : windParams2.w;
	float L = windParams2.y;

	// The gusts: where this is along the wind, less how far they have come.
	// Whole waves in a gust length, so the wrap of `travel` is never seen.
	float along = (dot(restPos.xz, dir) - travel) / L;
	float gust = 0.5 + 0.35 * sin(WIND_TAU * along) + 0.15 * sin(WIND_TAU * 3.0 * along + 1.7);

	// The lean: a push downwind, more in a gust, and the plant swinging about
	// it - mostly the swing: a steady lean is a plant that grew crooked.
	float phase = w.z * WIND_TAU;
	float rock = 0.6 * sin(t * WindRate(1108.0) + phase) + 0.25 * sin(t * WindRate(1760.0) + phase * 2.3);
	float lean = strength * (0.25 + 0.45 * gust) + 0.6 * strength * rock * (0.7 + 0.3 * gust);
	vec3 bend = vec3(dir.x, 0.0, dir.y) * (lean * w.x);

	// Flutter: across the wind and up and down, quick and small, each part of
	// a crown on its own (its phase from where it is, on the gusts' scale).
	float h = dot(restPos, vec3(0.37, 0.71, 0.53)) * (280.0 / L);
	float f1 = sin(t * WindRate(5867.0) + h + phase * 5.0);
	float f2 = sin(t * WindRate(8410.0) + h * 1.6 + phase * 3.0);
	// (matFlutter: the material's share of it, `wind <sway> <height> <flutter>`;
	// 1 unless given, 0 on a tree's bark)
	vec3 flutter = (vec3(-dir.y, 0.0, dir.x) * f1 + vec3(0.0, 0.6, 0.0) * f2)
		* (0.25 * w.y * strength * (0.5 + gust) * matFlutter);

	return (bend + flutter) * matWind;
}
#endif
