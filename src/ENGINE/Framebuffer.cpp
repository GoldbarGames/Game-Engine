#include "FrameBuffer.h"
#include "Shader.h"
#include "render/RenderDevice.h"

FrameBuffer::FrameBuffer(const Renderer& renderer, int screenWidth, int screenHeight)
{
	RenderDevice& device = Device();
	const FramebufferHandle fbo = device.CreateFramebuffer();
	framebufferObject = fbo.id;

	Texture* screenTexture = new Texture("");
	screenTexture->LoadTexture(textureColorBuffer, screenWidth, screenHeight);

	sprite = new Sprite(screenTexture, renderer.shaders[2]);
	sprite->keepPositionRelativeToCamera = true;
	sprite->keepScaleRelativeToCamera = true;

	device.AttachTexture(fbo, Attachment::Color0, TextureHandle(textureColorBuffer));

	// Depth+stencil as a TEXTURE (instead of a renderbuffer) so post-process
	// passes can sample the scene depth. Nearest filtering + clamp.
	renderBufferObject = 0;
	TextureDesc depthDesc;
	depthDesc.format = TextureFormat::Depth24Stencil8;
	depthDesc.width = screenWidth;
	depthDesc.height = screenHeight;
	depthDesc.filter = TextureFilter::Nearest;
	depthDesc.wrap = TextureWrap::ClampToEdge;
	depthTexture = device.CreateTexture(depthDesc).id;
	device.AttachTexture(fbo, Attachment::DepthStencil, TextureHandle(depthTexture));

	// "Is-character" mask (R8), color attachment 1. Only written when a pass
	// enables draw buffer 1 (the character billboards); the default draw-buffer
	// state writes attachment 0 only, so this stays dormant for 2D content and
	// non-character 3D geometry. The toon-outline post-process samples it to skip
	// character sprites (see scene3d_edge.frag / Game::Render / Character3D::Render).
	TextureDesc maskDesc;
	maskDesc.format = TextureFormat::R8;
	maskDesc.width = screenWidth;
	maskDesc.height = screenHeight;
	maskDesc.filter = TextureFilter::Nearest;
	maskDesc.wrap = TextureWrap::ClampToEdge;
	maskTexture = device.CreateTexture(maskDesc).id;
	device.AttachTexture(fbo, Attachment::Color1, TextureHandle(maskTexture));

	std::string error;
	if (!device.IsFramebufferComplete(fbo, &error))
	{
		std::cout << "ERROR::FRAMEBUFFER:: " << error << std::endl;
	}

	device.BindFramebuffer(FramebufferHandle());
}

FrameBuffer::~FrameBuffer()
{
	RenderDevice& device = Device();
	TextureHandle depth(depthTexture), mask(maskTexture);
	FramebufferHandle fbo(framebufferObject);
	device.DestroyTexture(depth);
	device.DestroyTexture(mask);
	device.DestroyFramebuffer(fbo);

	// Necessary to delete this here because it's not managed by the SpriteManager
	if (sprite->texture != nullptr)
		delete_it(sprite->texture);

	if (sprite != nullptr)
		delete_it(sprite);
}
