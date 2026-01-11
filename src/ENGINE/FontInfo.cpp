#include "FontInfo.h"
#include <SDL2/SDL_ttf.h>
#include <stdexcept>

FontInfo::FontInfo(const std::string& f, int s)
{
	size = s;
	regular = TTF_OpenFont(f.c_str(), size);
	pathRegular = f;

	if (regular == nullptr)
		throw std::invalid_argument(std::string("Tried to create font with a null pointer: ") + f.c_str());
}

FontInfo::~FontInfo()
{
	if (regular != nullptr)
	{		
		TTF_CloseFont(regular);
		regular = nullptr;
	}

	if (bold != nullptr)
	{
		TTF_CloseFont(bold);
		bold = nullptr;
	}

	if (italics != nullptr)
	{
		TTF_CloseFont(italics);
		italics = nullptr;
	}

	if (boldItalics != nullptr)
	{
		TTF_CloseFont(boldItalics);
		boldItalics = nullptr;
	}
	
}

const int FontInfo::GetFontSize()
{
	if (size > 0)
		return size;
	else
		return 1;
}

void FontInfo::ChangeFontSize(int newSize)
{
	size = newSize;
	SetRegularFont(pathRegular.c_str());
	SetBoldFont(pathBold.c_str());
	SetItalicsFont(pathItalics.c_str());
	SetBoldItalicsFont(pathBoldItalics.c_str());
}

void FontInfo::SetRegularFont(const std::string& f)
{
	if (f.c_str() != nullptr)
	{
		pathRegular = f;
		regular = TTF_OpenFont(f.c_str(), size);
	}
}

void FontInfo::SetBoldFont(const std::string& f)
{
	pathBold = f;
	bold = TTF_OpenFont(f.c_str(), size);
}

void FontInfo::SetItalicsFont(const std::string& f)
{
	pathItalics = f;
	italics = TTF_OpenFont(f.c_str(), size);
}

void FontInfo::SetBoldItalicsFont(const std::string& f)
{
	pathBoldItalics = f;
	boldItalics = TTF_OpenFont(f.c_str(), size);
}

TTF_Font* FontInfo::GetRegularFont()
{
	return regular;
}

TTF_Font* FontInfo::GetBoldFont()
{
	if (bold == nullptr)
		return regular;

	return bold;
}

TTF_Font* FontInfo::GetItalicsFont()
{
	if (italics == nullptr)
		return regular;

	return italics;
}

TTF_Font* FontInfo::GetBoldItalicsFont()
{
	if (boldItalics == nullptr)
		return regular;

	return boldItalics;
}
