#include "Sprite.h"
#include "Renderer.h"

class FrameBuffer
{
public:
	unsigned int framebufferObject;
	unsigned int renderBufferObject;
	unsigned int textureColorBuffer;

	Sprite* sprite = nullptr;

	FrameBuffer(const Renderer& renderer, int screenWidth, int screenHeight);
	~FrameBuffer();
};