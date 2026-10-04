#ifndef KINJO_TONEMAP_GLSL
#define KINJO_TONEMAP_GLSL
// Tonemapping (Phase 1.5 item 2): scene-linear HDR colour (Rec.709 primaries)
// in, display-linear [0,1] out - encode it with LinearToSrgb (target.glsl)
// afterwards. Chosen per project with `tonemap` in data/config/renderer.dat.
//
//   0 none        clamp at 1.0 (item 1's behaviour: SDR content is exact)
//   1 agx         AgX base (Troy Sobotka; the polynomial fit by Benjamin
//                 Wrensch). Neutral and flat - meant to be graded afterwards;
//                 bright saturated colours desaturate toward white the way
//                 film does, instead of clipping to a flat hue.
//   2 agx_punchy  AgX with the "punchy" look (more contrast and saturation):
//                 the engine default, closest to authored art.
//   3 aces        ACES RRT+ODT, Stephen Hill's fit. Contrasty, with ACES's
//                 characteristic hue shifts toward bright highlights.

// --- AgX ---------------------------------------------------------------
vec3 AgxContrastApprox(vec3 x)
{
	vec3 x2 = x * x;
	vec3 x4 = x2 * x2;
	return 15.5 * x4 * x2
		- 40.14 * x4 * x
		+ 31.96 * x4
		- 6.868 * x2 * x
		+ 0.4298 * x2
		+ 0.1191 * x
		- 0.00232;
}

vec3 AgxEncode(vec3 c)
{
	const mat3 inset = mat3(
		0.842479062253094, 0.0423282422610123, 0.0423756549057051,
		0.0784335999999992, 0.878468636469772, 0.0784336,
		0.0792237451477643, 0.0791661274605434, 0.879142973793104);
	const float minEv = -12.47393;
	const float maxEv = 4.026069;
	c = inset * c;
	c = clamp(log2(max(c, vec3(1e-10))), minEv, maxEv);
	c = (c - minEv) / (maxEv - minEv);
	return AgxContrastApprox(c);
}

// ASC CDL look on AgX's encoded output.
vec3 AgxPunchy(vec3 c)
{
	float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
	c = pow(max(c, vec3(0.0)), vec3(1.35));
	return luma + 1.4 * (c - luma);
}

vec3 AgxDecode(vec3 c)
{
	const mat3 outset = mat3(
		1.19687900512017, -0.0528968517574562, -0.0529716355144438,
		-0.0980208811401368, 1.15190312990417, -0.0980434501171241,
		-0.0990297440797205, -0.0989611768448433, 1.15107367264116);
	c = outset * c;
	return pow(max(c, vec3(0.0)), vec3(2.2));   // to display-linear
}

// --- ACES (Stephen Hill's fit of the RRT + sRGB ODT) ---------------------
vec3 AcesFitted(vec3 c)
{
	const mat3 inputMat = mat3(
		0.59719, 0.07600, 0.02840,
		0.35458, 0.90834, 0.13383,
		0.04823, 0.01566, 0.83777);
	const mat3 outputMat = mat3(
		1.60475, -0.10208, -0.00327,
		-0.53108, 1.10813, -0.07276,
		-0.07367, -0.00605, 1.07602);
	c = inputMat * c;
	vec3 a = c * (c + 0.0245786) - 0.000090537;
	vec3 b = c * (0.983729 * c + 0.4329510) + 0.238081;
	c = outputMat * (a / b);
	return clamp(c, 0.0, 1.0);
}

// Each operator is calibrated so that exposure 1.0 puts mid-grey (0.18) where
// the untonemapped image has it (sRGB 0.46): switching tonemapper changes
// contrast and highlight handling, not overall brightness, so a scene's
// exposure keeps its meaning. (Constants solved numerically from the curves.)
vec3 Tonemap(vec3 c, int op)
{
	if (op == 1)
		return clamp(AgxDecode(AgxEncode(c * 0.8084)), 0.0, 1.0);
	if (op == 2)
		return clamp(AgxDecode(AgxPunchy(AgxEncode(c * 1.4313))), 0.0, 1.0);
	if (op == 3)
		return AcesFitted(c * 1.4619);
	return clamp(c, 0.0, 1.0);
}
#endif
