#include "FrameBuffer.h"
#include "Shader.h"

FrameBuffer::FrameBuffer(const Renderer& renderer, int screenWidth, int screenHeight)
{
	glGenFramebuffers(1, &framebufferObject);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);

	Texture* screenTexture = new Texture("");
	screenTexture->LoadTexture(textureColorBuffer, screenWidth, screenHeight);

	sprite = new Sprite(screenTexture, renderer.shaders[2]);
	sprite->keepPositionRelativeToCamera = true;
	sprite->keepScaleRelativeToCamera = true;

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

	// Depth+stencil as a TEXTURE (instead of a renderbuffer) so post-process
	// passes can sample the scene depth. Nearest filtering + clamp.
	renderBufferObject = 0;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, screenWidth, screenHeight, 0,
		GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	// "Is-character" mask (R8), color attachment 1. Only written when a pass
	// enables draw buffer 1 (the character billboards); the default draw-buffer
	// state writes attachment 0 only, so this stays dormant for 2D content and
	// non-character 3D geometry. The toon-outline post-process samples it to skip
	// character sprites (see scene3d_edge.frag / Game::Render / Character3D::Render).
	glGenTextures(1, &maskTexture);
	glBindTexture(GL_TEXTURE_2D, maskTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, screenWidth, screenHeight, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, maskTexture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
	}
		
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
	if (depthTexture != 0)
		glDeleteTextures(1, &depthTexture);
	if (maskTexture != 0)
		glDeleteTextures(1, &maskTexture);
	if (framebufferObject != 0)
		glDeleteFramebuffers(1, &framebufferObject);

	// Necessary to delete this here because it's not managed by the SpriteManager
	if (sprite->texture != nullptr)
		delete_it(sprite->texture);

	if (sprite != nullptr)
		delete_it(sprite);
}