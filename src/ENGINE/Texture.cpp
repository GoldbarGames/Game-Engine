#include "Texture.h"
#include "render/RenderDevice.h"
#include <iostream>

// Kept for binary compatibility with games built against the old header; the
// bind cache now lives in the render device.
int Texture::lastTextureID = -1;
int Texture::lastActiveTexture = -1;

Texture::Texture(const std::string& path)
{
	textureID = 0;
	width = 0;
	height = 0;
	filePath = path;
}

Texture::~Texture()
{
	ClearTexture();
}

// This function is used exclusively for loading textures on imported 3D meshes
bool Texture::LoadTexture()
{
	//TODO: Make this work with PhysFS, possibly refactor it entirely

	SDL_Surface* surface = IMG_Load(filePath.c_str());
	if (surface == nullptr)
	{
		surface = IMG_Load("assets/gui/white.png");
		std::cout << "FAILED TO LOAD TEXTURE: " << filePath << std::endl;
		return false;
	}

	LoadTexture(surface);

	SDL_FreeSurface(surface);
	return true;
}

// An empty RGBA render-target colour texture (framebuffers): linear, clamped.
void Texture::LoadTexture(unsigned int& buffer, int w, int h)
{
	width = w;
	height = h;
	TextureDesc desc;
	desc.width = w;
	desc.height = h;
	desc.filter = TextureFilter::Linear;
	desc.wrap = TextureWrap::ClampToEdge;
	buffer = Device().CreateTexture(desc, nullptr).id;
	textureID = buffer;
}

void Texture::LoadTexture(SDL_Surface* surface, bool reset, Filter filter)
{
	if (reset)
	{
		TextureHandle old(textureID);
		Device().DestroyTexture(old);
	}

	// Convert surface to RGBA format to handle BGR/RGB and indexed color issues
	SDL_Surface* convertedSurface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
	if (convertedSurface == nullptr)
	{
		std::cout << "WARNING: Could not convert surface to RGBA, using original format" << std::endl;
		convertedSurface = surface;
	}

	width = convertedSurface->w;
	height = convertedSurface->h;

	TextureDesc desc;
	desc.width = width;
	desc.height = height;
	desc.wrap = TextureWrap::Repeat;
	if (filter == Filter::Smooth)
	{
		// Trilinear minification actually uses the generated mipmaps (nearest
		// ignored them, which made scaled-down text grainy), and anisotropic
		// sampling keeps surfaces seen at oblique angles (2.5D floors) sharp.
		desc.filter = TextureFilter::Trilinear;
		desc.generateMipmaps = true;
		desc.maxAnisotropy = 8.0f;
	}
	else
	{
		// Point: crisp pixel art, no mipmaps (nearest never samples them)
		desc.filter = TextureFilter::Nearest;
	}
	textureID = Device().CreateTexture(desc, convertedSurface->pixels).id;

	// Free the converted surface if we created one
	if (convertedSurface != surface)
	{
		SDL_FreeSurface(convertedSurface);
	}
}

void Texture::UseTexture(int unit)
{
	// Accept a raw GL_TEXTUREn (0x84C0 + n) too: binaries built against the
	// old header pass GL_TEXTURE0 as the default argument.
	const int kGLTexture0 = 0x84C0;
	const int index = (unit >= kGLTexture0) ? unit - kGLTexture0 : unit;
	Device().BindTexture((unsigned int)index, TextureHandle(textureID));
}

void Texture::ClearTexture()
{
	TextureHandle handle(textureID);
	Device().DestroyTexture(handle);
	textureID = 0;
	width = 0;
	height = 0;
}
