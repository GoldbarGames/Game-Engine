#version 330

// Depth of field, step 2 (src/ENGINE/render/DepthOfField.h), at half
// resolution: a "scatter as gather" disc blur after Dennis Gustafsson's
// single-pass bokeh. Samples spiral out by the golden angle; each one counts
// when its OWN blur is big enough to reach this pixel. Something behind this
// pixel may not blur over it by more than twice this pixel's own size, so an
// in-focus subject stays crisp against a blurred background - while a blurred
// foreground (in front, its full size) does spill over the subject.
// Alpha out: the largest blur that reached this pixel, which tells the
// composite where the blurred image must win even if this pixel is in focus.
#define KINJO_LINEAR_OUTPUT

#include "draw.glsl"

struct DrawData
{
	vec2  size;      // half-res size
	float maxBlur;   // half-res pixels
};
PER_DRAW(DrawData);

uniform sampler2D halfColor;   // rgb colour, a = signed blur size (dof_prepare.frag), linear filtered

out vec4 result;

const float GOLDEN_ANGLE = 2.39996323;
const float RADIUS_STEP = 0.9;   // smaller = more samples

void main()
{
	vec4 centre = texelFetch(halfColor, ivec2(gl_FragCoord.xy), 0);
	float centreSize = abs(centre.a);
	vec3 color = centre.rgb;
	float total = 1.0;
	float reached = centreSize;

	float radius = RADIUS_STEP;
	for (float angle = 0.0; radius < draw.maxBlur; angle += GOLDEN_ANGLE)
	{
		vec2 uv = (gl_FragCoord.xy + vec2(cos(angle), sin(angle)) * radius) / draw.size;
		vec4 s = textureLod(halfColor, uv, 0.0);
		float sampleSize = abs(s.a);
		if (s.a > centre.a)   // farther than this pixel (the blur size grows with depth)
			sampleSize = min(sampleSize, centreSize * 2.0);
		float m = smoothstep(radius - 0.5, radius + 0.5, sampleSize);
		color += mix(color / total, s.rgb, m);
		total += 1.0;
		reached = max(reached, sampleSize * m);
		radius += RADIUS_STEP / radius;
	}
	result = vec4(color / total, reached);
}
