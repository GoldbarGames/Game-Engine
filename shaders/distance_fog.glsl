#ifndef KINJO_DISTANCE_FOG_GLSL
#define KINJO_DISTANCE_FOG_GLSL
// Distance fog (src/ENGINE/render/DistanceFog.h): a surface fades to the fog
// colour with its distance from the camera - none nearer than distFogParams.x,
// all of it from .y on, linearly in between, scaled by .z (0 = off; between 0
// and 1 while the fog fades in or out). From the Scene block, the colour
// already in the target's space (linear in a linear-workflow project).
#include "scene.glsl"

float DistanceFogAmount(vec3 worldPos)
{
	if (distFogParams.z <= 0.0)
		return 0.0;
	float d = distance(worldPos, viewPos);
	return clamp((d - distFogParams.x) / max(distFogParams.y - distFogParams.x, 0.001), 0.0, 1.0) * distFogParams.z;
}

vec3 ApplyDistanceFog(vec3 color, vec3 worldPos)
{
	return mix(color, distFogColor.rgb, DistanceFogAmount(worldPos));
}
#endif
