#version 330

// Post-process toon outline: composite the 3D scene colour and overlay a solid
// outline wherever the scene DEPTH has a discontinuity (object silhouettes,
// where things meet, creases against the background). Works for ALL geometry
// regardless of mesh topology - unlike an inverted hull. Uses a depth Laplacian
// so smooth receding surfaces (a floor) don't false-trigger.

in vec2 TexCoord;
out vec4 color;

uniform sampler2D theTexture;   // 3D scene colour (unit 0)
uniform vec4 spriteColor;       // framebuffer sprite tint (usually white)
uniform sampler2D depthTex;     // scene depth (unit 1)
uniform sampler2D maskTex;      // "is-character" mask (unit 2): 1.0 = character sprite

uniform vec2 texelSize;         // 1 / screen size
uniform float nearPlane;
uniform float farPlane;
uniform float edgeThreshold;    // world-unit sensitivity (smaller = more edges)
uniform float thickness;        // outline width in pixels
uniform vec3 outlineColor;

float Linear(vec2 uv)
{
	float d = texture(depthTex, uv).r * 2.0 - 1.0;   // NDC z
	return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - d * (farPlane - nearPlane));
}

bool IsChar(vec2 uv)
{
	return texture(maskTex, uv).r > 0.5;
}

// Neighbour depth, but character pixels are treated as "same as centre" so a
// character silhouette never counts as a depth edge (no line on the sprite, no
// halo on the geometry/sky behind it). c = centre depth.
float NeighbourDepth(vec2 uv, float c)
{
	return IsChar(uv) ? c : Linear(uv);
}

void main()
{
	vec4 base = texture(theTexture, TexCoord) * spriteColor;

	vec2 t = texelSize * max(thickness, 1.0);
	float c = Linear(TexCoord);
	float l = NeighbourDepth(TexCoord - vec2(t.x, 0.0), c);
	float r = NeighbourDepth(TexCoord + vec2(t.x, 0.0), c);
	float u = NeighbourDepth(TexCoord + vec2(0.0, t.y), c);
	float d = NeighbourDepth(TexCoord - vec2(0.0, t.y), c);

	// Second difference (Laplacian): ~0 on smooth gradients, spikes at
	// silhouettes / depth jumps. Threshold grows with distance so far surfaces
	// need a bigger discontinuity to count as an edge.
	float lap = abs(l + r - 2.0 * c) + abs(u + d - 2.0 * c);
	float edge = step(edgeThreshold * (1.0 + c * 0.03), lap);

	// Don't outline the empty far background (nothing was drawn there).
	if (c >= farPlane * 0.98)
		edge = 0.0;

	// Never draw the outline on a character sprite itself.
	if (IsChar(TexCoord))
		edge = 0.0;

	color = vec4(mix(base.rgb, outlineColor, edge), base.a);
}
