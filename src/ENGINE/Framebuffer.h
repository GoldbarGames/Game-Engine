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

	Sprite* sprite = nullptr;

	FrameBuffer(const Renderer& renderer, int screenWidth, int screenHeight);
	~FrameBuffer();
};