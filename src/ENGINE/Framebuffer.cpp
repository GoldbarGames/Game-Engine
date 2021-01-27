#include "Framebuffer.h"
#include "Shader.h"

FrameBuffer::FrameBuffer(const Renderer& renderer, int screenWidth, int screenHeight)
{
	glGenFramebuffers(1, &framebufferObject);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject);

	Texture* screenTexture = new Texture("");
	screenTexture->LoadTexture(textureColorBuffer, screenWidth, screenHeight);

	sprite = new Sprite(screenTexture, renderer.shaders[ShaderName::GUI]);
	sprite->keepPositionRelativeToCamera = true;
	sprite->keepScaleRelativeToCamera = true;

	// attach it to currently bound framebuffer object
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorBuffer, 0);

	glGenRenderbuffers(1, &renderBufferObject);
	glBindRenderbuffer(GL_RENDERBUFFER, renderBufferObject);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, screenWidth, screenHeight);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderBufferObject);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

FrameBuffer::~FrameBuffer()
{
	// Necessary to delete this here because it's not managed by the SpriteManager
	if (sprite->texture != nullptr)
		delete_it(sprite->texture);

	if (sprite != nullptr)
		delete_it(sprite);
}