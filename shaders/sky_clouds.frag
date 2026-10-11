#version 330

// A LAYER OF CLOUD over the sky's panorama (src/ENGINE/SkyClouds.h), worked out
// per pixel each frame - the same sums TrainRails' tools/make_sky.py paints its
// panoramas with, so the clouds look as the painted ones did, and move.
//
// Each pixel's direction is followed up to the layer and noise is read where it
// lands: a cloud overhead is big, the same cloud near the horizon small and
// crowded behind its neighbours. A sheet (cirrus, an overcast deck, scud) is
// read once. A heaped layer (cumulus) is a slab the ray is marched through,
// laying up the cloud it passes front to back: overhead it looks up into flat
// bases, low down it meets the heaps from the side, tops in the sun.

in vec3 Dir;
flat in vec3 CamPos;

layout(location = 0) out vec4 color;

uniform sampler2D cloudNoise;   // unit 0: SkyClouds.cpp's table of values, 256 square, repeating

#include "draw.glsl"

struct DrawData
{
	vec4 p0;   // heaped (0/1), height, size, cover
	vec4 p1;   // soft, warp, opacity, depth
	vec4 p2;   // stretch x, stretch y, angle, haze distance
	vec4 p3;   // lit rgb, silver
	vec4 p4;   // shade rgb, thick dark
	vec4 p5;   // drift x, drift z, base dark, noise seed (whole) + this frame's stagger turn (fraction)
	vec4 p6;   // toward the sun xyz, the sky's radius
	vec4 p7;   // toward the rim light xyz, glare
};
PER_DRAW(DrawData);

// Linear-aware (render/ColorPipeline.h): the colours are worked in sRGB, as the
// panoramas are painted, and converted for the target at the end.
#include "target.glsl"

const float TABLE = 256.0;

// Value noise, 0..1, read smoothly between the table's lattice points. Each
// octave reads its own part of the table. The four corners are fetched and
// blended here, not by the texture unit: its filtering weights have 8 bits, so
// across a cell magnified to hundreds of pixels the value climbed in 256
// stairs, and the clouds' steep thresholds drew them as fine stripes.
float Value(vec2 p, float octave)
{
	p += vec2(37.3, 59.7) * octave + vec2(13.1, 7.7) * floor(draw.p5.w);
	vec2 i = floor(p);
	vec2 f = p - i;
	f = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);   // quintic: no creases at the lattice
#ifdef KINJO_GL4
	// The 2x2 texels round the shared corner of cell i: w (0,0), z (1,0), x (0,1), y (1,1).
	vec4 g = textureGather(cloudNoise, (i + 1.0) / TABLE);
	return mix(mix(g.w, g.z, f.x), mix(g.x, g.y, f.x), f.y);
#else
	ivec2 a = ivec2(mod(i, TABLE));
	ivec2 b = ivec2(mod(i + 1.0, TABLE));
	return mix(mix(texelFetch(cloudNoise, a, 0).r, texelFetch(cloudNoise, ivec2(b.x, a.y), 0).r, f.x),
		mix(texelFetch(cloudNoise, ivec2(a.x, b.y), 0).r, texelFetch(cloudNoise, b, 0).r, f.x), f.y);
#endif
}

// Fractal noise, 0..1, each octave faded to its mean where the pixel's
// footprint (in the same units) is too big to show it - near the horizon a
// pixel covers miles of cloud, and its fine detail would only shimmer.
float Fbm(vec2 p, int octaves, float footprint, float first)
{
	float total = 0.0, norm = 0.0, amp = 1.0, f = 1.0;
	for (int o = 0; o < octaves; o++)
	{
		float w = clamp(1.5 - f * footprint * 2.0, 0.0, 1.0);
		float v = (w > 0.0) ? Value(p * f, first + float(o)) : 0.5;
		total += amp * (0.5 + w * (v - 0.5));
		norm += amp;
		amp *= 0.5;
		f *= 2.03;
	}
	return total / norm;
}

// A pixel's footprint on the layer, in noise cells: its angle times the
// distance, over how flat it grazes the layer (as tools/make_sky.py has it).
// Not from the screen's derivatives, which are shared by each 2x2 block of
// pixels: where a thin cloud sits at its threshold, the octaves they fade
// stepped from block to block and the cloud came out dithered.
const float PIXEL = 0.0014;   // radians: about a pixel at 1280 wide and 55 degrees

void main()
{
	vec3 d = normalize(Dir);
	float up = -d.y;                             // visual up is -Y
	if (up <= 0.0)
		discard;

	float H = draw.p0.y;
	float size = draw.p0.z;
	float cover = draw.p0.w;
	float soft = draw.p1.x;
	float warp = draw.p1.y;
	vec3 toSun = normalize(draw.p6.xyz);
	// Where the camera is under the layer, less how far the wind has blown it.
	vec2 origin = CamPos.xz - draw.p5.xy;
	float yy = max(up, 0.012);

	float density, lit, n = 0.0, dist;
	if (draw.p0.x > 0.5)
	{
		// HEAPED: the slab from the layer's base to `depth` above it. A cloud
		// stands up from its base as tall as the noise is strong there.
		float depth = draw.p1.w;
		float d0 = H / yy;
		float d1 = H * (1.0 + depth) / yy;
		vec2 base = (origin + d.xz * d0) / size;
		float fp0 = PIXEL * (d0 / size) / yy;
		// Swirled once, where the ray enters: slow enough to hold for its length.
		vec2 swirl = warp * 2.0 * (vec2(Fbm(base * 0.3 + vec2(11.7, 5.3), 4, fp0 * 0.3, 20.0),
			Fbm(base * 0.3 + vec2(47.1, 23.9), 4, fp0 * 0.3, 24.0)) - 0.5);
		const int STEPS = 14;
		float seg = (d1 - d0) / H / float(STEPS);
		float stepCells = (d1 - d0) * length(d.xz) / float(STEPS) / size;   // a step's length on the layer, in cells
		// Each ray starts its march part of a step in: in step, the steps show
		// as slices stacked up each cloud. The part varies smoothly over the
		// sky (noise), so the slices dissolve into soft waves. Staggered pixel
		// by pixel instead (interleaved gradient noise, as tools/make_sky.py
		// does), it showed as a checkered dither: the panorama painter averages
		// it away by painting at twice the size, which a frame can't.
		float stagger = Value(base * 2.3, 30.0);
		float trans = 1.0, light = 0.0;
		for (int k = 0; k < STEPS; k++)
		{
			float h = (float(k) + stagger) / float(STEPS);
			float dk = d0 + (d1 - d0) * h;
			// Detail finer than a step along the ray can't be followed by the
			// march: faded out like detail finer than a pixel, or it aliases
			// into stripes across the clouds (long steps, low in the sky).
			float nk = Fbm((origin + d.xz * dk) / size + swirl, 7, max(fp0 * dk / d0, stepCells), 0.0);
			float tall = (nk - cover) / soft;           // the column's height here, 0..1 of the slab
			float inside = smoothstep(-0.35, 0.35, tall - h);
			float a = 1.0 - exp(-9.0 * inside * seg);
			// Lit by height - tops in the sun, bases in shade - and a little
			// brighter where it is thickest: the crowns of the heaps.
			float crown = smoothstep(cover + soft * 0.4, cover + soft * 1.6, nk);
			light += trans * a * (0.30 + 0.60 * pow(h, 0.7) + 0.18 * crown);
			trans *= 1.0 - a;
		}
		density = (1.0 - trans) * smoothstep(0.0, 0.03, up);
		lit = light / max(1.0 - trans, 1e-4);
		lit *= 1.0 - 0.30 * smoothstep(0.3, 1.0, dot(d, toSun));   // toward the sun, their shadowed sides
		lit = clamp(lit, 0.0, 1.0);
		dist = d0;
	}
	else
	{
		// A SHEET, drawn out along `angle` by `stretch` (cirrus).
		dist = H / yy;
		vec2 w = (origin + d.xz * dist) / size;
		float ca = cos(draw.p2.z), sa = sin(draw.p2.z);
		vec2 p = vec2(w.x * ca - w.y * sa, w.x * sa + w.y * ca) * draw.p2.xy;
		float fp = PIXEL * (dist / size) / yy * max(draw.p2.x, draw.p2.y);
		vec2 q = p + warp * 2.0 * (vec2(Fbm(p * 0.3 + vec2(11.7, 5.3), 4, fp * 0.3, 20.0),
			Fbm(p * 0.3 + vec2(47.1, 23.9), 4, fp * 0.3, 24.0)) - 0.5);
		n = Fbm(q, 8, fp, 0.0);
		density = smoothstep(cover, cover + soft, n) * smoothstep(0.0, 0.035, up);
		lit = 1.0 - draw.p4.w * smoothstep(0.45, 0.85, n);   // a deck's heavier places darker
	}

	vec3 litCol = draw.p3.rgb;
	vec3 col = mix(draw.p4.rgb, litCol, lit);
	if (draw.p0.x < 0.5)
	{
		// Darker in its thick middles (scud seen from underneath).
		float core = smoothstep(cover + soft, cover + soft + 0.22, n);
		col *= 1.0 - draw.p5.z * core;
	}
	// Thin edges near the sun (or the moon) catch its light: the silver lining.
	if (draw.p3.a > 0.0)
	{
		float rim = clamp(density * (1.0 - density) * 4.0, 0.0, 1.0);
		col += draw.p3.a * rim * exp((dot(d, normalize(draw.p7.xyz)) - 1.0) * 14.0) * litCol;
	}
	// The sun's glare is in the eye and the air, in front of any cloud: the
	// panorama's own is under this layer, so it is put back where it covers.
	if (draw.p7.w > 0.0)
	{
		float ang = acos(clamp(dot(d, toSun), -1.0, 1.0));
		col += draw.p7.w * (0.9 * exp(-ang / 0.006) + 0.32 * exp(-ang / 0.035) + 0.10 * exp(-ang / 0.20)) * litCol;
	}

	// Into the haze with distance: the panorama's own horizon shows through.
	float haze = 1.0 - exp(-dist / draw.p2.w);
	float a = clamp(density * draw.p1.z * (1.0 - haze), 0.0, 1.0);
	col = pow(max(col, vec3(0.0)), vec3(1.0 / 1.05));          // as the panoramas are written (make_sky.py)
	color = vec4(TargetColor(col) * a, a);
}
