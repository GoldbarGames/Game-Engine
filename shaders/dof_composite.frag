#version 330

// Depth of field, step 3 (src/ENGINE/render/DepthOfField.h): the sharp
// full-resolution image where the pixel is in focus and nothing blurred
// reaches it, the half-resolution blur elsewhere, blended over a pixel or two.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"
#include "dof_common.glsl"

struct DrawData
{
	vec4 lens;     // focus, aperture (full-res px), max blur (full-res px), unused
	vec4 planes;   // near, far
	vec2 size;     // full-res size
};
PER_DRAW(DrawData);

uniform sampler2D srcColor;    // the full-resolution world image
uniform sampler2D srcDepth;    // its depth
uniform sampler2D blurColor;   // dof_blur.frag's result (half res, linear filtered)

out vec4 result;

void main()
{
	ivec2 p = ivec2(gl_FragCoord.xy);
	vec4 sharp = texelFetch(srcColor, p, 0);
	float depth = DofViewDepth(texelFetch(srcDepth, p, 0).r, draw.planes.x, draw.planes.y);
	float coc = abs(DofCoc(depth, draw.lens.x, draw.lens.y, draw.lens.z));
	vec4 blurred = textureLod(blurColor, gl_FragCoord.xy / draw.size, 0.0);
	float t = smoothstep(1.0, 2.5, max(coc, blurred.a * 2.0));   // blurred.a is in half-res pixels
	result = vec4(mix(sharp.rgb, blurred.rgb, t), sharp.a);
}
