#version 330

// Depth of field, step 1 (src/ENGINE/render/DepthOfField.h): the world image
// at half resolution, with each pixel's signed blur size (in half-res pixels)
// in alpha - from the NEAREST of its four depths, so the edge of a blurred
// foreground object keeps its own blur.
// Writes data (alpha is not coverage): KINJO_LINEAR_OUTPUT keeps the linear
// workflow from adapting it.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"
#include "dof_common.glsl"

struct DrawData
{
	vec4 lens;      // focus, aperture (half-res px), max blur (half-res px), unused
	vec4 planes;    // near, far
	vec2 dstSize;   // half-res size
};
PER_DRAW(DrawData);

uniform sampler2D srcColor;   // the full-resolution world image
uniform sampler2D srcDepth;   // its depth

out vec4 result;

void main()
{
	ivec2 base = ivec2(gl_FragCoord.xy) * 2;
	ivec2 last = textureSize(srcColor, 0) - 1;
	vec3 color = vec3(0.0);
	float nearest = 1.0;
	for (int i = 0; i < 4; i++)
	{
		ivec2 p = min(base + ivec2(i & 1, i >> 1), last);
		vec3 c = texelFetch(srcColor, p, 0).rgb;
		if (any(isnan(c)) || any(isinf(c)))
			c = vec3(0.0);
		color += max(c, vec3(0.0));
		nearest = min(nearest, texelFetch(srcDepth, p, 0).r);
	}
	float depth = DofViewDepth(nearest, draw.planes.x, draw.planes.y);
	result = vec4(color * 0.25, DofCoc(depth, draw.lens.x, draw.lens.y, draw.lens.z));
}
