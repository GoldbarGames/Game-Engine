#include "Sprite.h"
#include "Renderer.h"

class FrameBuffer
{
public:
	unsigned int framebufferObject;
	unsigned int renderBufferObject;
	unsigned int textureColorBuffer;
	// Sampleable depth (24) + stencil (8) texture, so post-process passes (e.g.
	// the toon depth-edge outline) can read the scene depth.
	unsigned int depthTexture = 0;
	// Second color attachment (R8): an "is-character" mask the toon outline reads
	// so it can skip character billboards. Written only when a pass enables draw
	// buffer 1 (Character3D::Render); dormant otherwise (default draws to 0 only).
	unsigned int maskTexture = 0;

	Sprite* sprite = nullptr;

	FrameBuffer(const Renderer& renderer, int screenWidth, int screenHeight);
	~FrameBuffer();
};