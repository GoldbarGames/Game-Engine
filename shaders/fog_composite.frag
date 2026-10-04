#version 330

// Volumetric fog, step 2 (src/ENGINE/render/VolumetricFog.h): the half-
// resolution fog brought up to full resolution and laid over the world. Each
// pixel blends its four nearest fog samples by distance AND by how close their
// depth is to its own, so fog behind an object doesn't bleed onto its edge.
// Out: a premultiplied layer (rgb = light scattered in, a = 1 - transmittance)
// blended with BlendMode::Premultiplied: world * transmittance + light, and
// the world's alpha raised by the fog's opacity - so the fog also fills what
// nothing was drawn over (the empty background, hairline cracks).
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"

struct DrawData
{
	vec4 planes;     // near, far
	vec2 fullSize;
	vec2 halfSize;
};
PER_DRAW(DrawData);

uniform sampler2D fogTexture;   // rgb scattered light, a transmittance (half res)
uniform sampler2D sceneDepth;   // full-resolution depth

out vec4 result;

float ViewDepth(float d)
{
	float z = d * 2.0 - 1.0;
	return (2.0 * draw.planes.x * draw.planes.y) / (draw.planes.y + draw.planes.x - z * (draw.planes.y - draw.planes.x));
}

void main()
{
	ivec2 p = ivec2(gl_FragCoord.xy);
	float centre = ViewDepth(texelFetch(sceneDepth, p, 0).r);

	// Fog texel q covers full-res pixels 2q..2q+1 and was marched at 2q.
	vec2 halfPos = (gl_FragCoord.xy - 1.0) * 0.5;
	ivec2 base = ivec2(floor(halfPos));
	vec2 f = halfPos - vec2(base);
	ivec2 lastHalf = ivec2(draw.halfSize) - 1;
	ivec2 lastFull = ivec2(draw.fullSize) - 1;

	vec4 sum = vec4(0.0);
	float weightSum = 0.0;
	for (int i = 0; i < 4; i++)
	{
		ivec2 o = ivec2(i & 1, i >> 1);
		ivec2 q = clamp(base + o, ivec2(0), lastHalf);
		float bilinear = (o.x == 1 ? f.x : 1.0 - f.x) * (o.y == 1 ? f.y : 1.0 - f.y);
		float d = ViewDepth(texelFetch(sceneDepth, min(q * 2, lastFull), 0).r);
		float similar = 1.0 / (1.0e-3 + abs(d - centre) / max(centre, 1.0));
		float w = (bilinear + 1.0e-3) * similar;
		sum += texelFetch(fogTexture, q, 0) * w;
		weightSum += w;
	}
	vec4 fog = sum / max(weightSum, 1.0e-6);
	result = vec4(fog.rgb, 1.0 - fog.a);
}
