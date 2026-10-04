#version 330

// Bloom, upsample step (Phase 1.5 item 3; src/ENGINE/render/ColorPipeline.cpp).
// A 3x3 tent filter over the smaller level, ADDED (additive blending) onto the
// next larger one, so the top level ends up holding every level's blur - wide
// soft glow plus tight glow, with no single-blur halo shape.

#include "draw.glsl"
#include "target.glsl"   // linear-aware: works on linear HDR light

struct DrawData
{
	vec2 srcTexel;   // 1 / source (smaller level) size
	vec2 dstSize;    // the level being added to, in pixels
};
PER_DRAW(DrawData);

uniform sampler2D srcTexture;
out vec4 color;

void main()
{
	vec2 uv = gl_FragCoord.xy / draw.dstSize;
	vec2 t = draw.srcTexel;

	vec3 a = texture(srcTexture, uv + t * vec2(-1.0,  1.0)).rgb;
	vec3 b = texture(srcTexture, uv + t * vec2( 0.0,  1.0)).rgb;
	vec3 c = texture(srcTexture, uv + t * vec2( 1.0,  1.0)).rgb;
	vec3 d = texture(srcTexture, uv + t * vec2(-1.0,  0.0)).rgb;
	vec3 e = texture(srcTexture, uv).rgb;
	vec3 f = texture(srcTexture, uv + t * vec2( 1.0,  0.0)).rgb;
	vec3 g = texture(srcTexture, uv + t * vec2(-1.0, -1.0)).rgb;
	vec3 h = texture(srcTexture, uv + t * vec2( 0.0, -1.0)).rgb;
	vec3 i = texture(srcTexture, uv + t * vec2( 1.0, -1.0)).rgb;

	vec3 up = e * 4.0 + (b + d + f + h) * 2.0 + (a + c + g + i);
	color = vec4(up / 16.0, 1.0);
}
