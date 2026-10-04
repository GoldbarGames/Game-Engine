#include "Texture.h"
#include "render/RenderDevice.h"
#include "render/TextureFiles.h"
#include <iostream>
#include <vector>

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

	// A KTX2 file: asked for by name, or found beside the requested image.
	const std::string file = PreferredTextureFile(filePath);
	if (IsKtx2Path(file))
	{
		std::vector<unsigned char> bytes;
		if (ReadFileBytes(file, bytes) && LoadFromFileData(bytes.data(), bytes.size(), Filter::Point, false))
			return true;
	}

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
	LoadTexture(surface, reset, filter, false);
}

void Texture::LoadTexture(SDL_Surface* surface, bool reset, Filter filter, bool srgb)
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
	desc.format = srgb ? TextureFormat::SRGB8_A8 : TextureFormat::RGBA8;
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
		desc.maxAnisotropy = TextureAnisotropy();   // renderer.dat `anisotropy`, default 8
	}
	else
	{
		// Point: crisp pixel art, no mipmaps (nearest never samples them)
		desc.filter = TextureFilter::Nearest;
	}
	textureID = Device().CreateTexture(desc, convertedSurface->pixels).id;
	ForgetTextureFormat(this);   // plain RGBA now

	// Free the converted surface if we created one
	if (convertedSurface != surface)
	{
		SDL_FreeSurface(convertedSurface);
	}
}

bool Texture::LoadFromFileData(const void* bytes, size_t size, Filter filter, bool srgb)
{
	if (bytes == nullptr || size == 0)
		return false;

	if (!IsKtx2(bytes, size))
	{
		SDL_RWops* rw = SDL_RWFromConstMem(bytes, (int)size);
		SDL_Surface* surface = (rw != nullptr) ? IMG_Load_RW(rw, 1) : nullptr;
		if (surface == nullptr)
			return false;
		LoadTexture(surface, textureID != 0, filter, srgb);
		SDL_FreeSurface(surface);
		return true;
	}

	Ktx2Image image;
	std::string error;
	if (!ParseKtx2((const unsigned char*)bytes, size, image, error))
	{
		std::cout << "Texture " << filePath << ": " << error << std::endl;
		return false;
	}
	const TextureFormat format = WithColorSpace(image.format, srgb);
	if (!Device().SupportsTextureFormat(format))
	{
		std::cout << "Texture " << filePath << ": this GPU can't sample " << TextureFormatName(format)
			<< " textures" << std::endl;
		return false;
	}

	TextureDesc desc;
	desc.format = format;
	desc.width = image.width;
	desc.height = image.height;
	desc.wrap = TextureWrap::Repeat;
	if (filter == Filter::Smooth)
	{
		// The file's own mip chain (a single-level RGBA8 file gets one built).
		desc.filter = TextureFilter::Trilinear;
		desc.generateMipmaps = true;
		desc.maxAnisotropy = TextureAnisotropy();
	}
	else
	{
		desc.filter = TextureFilter::Nearest;
	}
	const TextureHandle handle = Device().CreateTextureLevels(desc, image.levels.data(), (int)image.levels.size());
	if (!handle)
		return false;

	if (textureID != 0)
	{
		TextureHandle old(textureID);
		Device().DestroyTexture(old);
	}
	textureID = handle.id;
	width = image.width;
	height = image.height;
	RememberTextureFormat(this, format);
	return true;
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
	ForgetTextureFormat(this);
	textureID = 0;
	width = 0;
	height = 0;
}
