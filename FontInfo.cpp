#include "FontInfo.h"
#include <SDL_ttf.h>
#include <stdexcept>

FontInfo::FontInfo(const char* f, int s)
{
	size = s;
	regular = TTF_OpenFont(f, size);
	pathRegular = f;

	if (regular == nullptr)
		throw std::invalid_argument("Tried to create font with null pointer");
}

FontInfo::~FontInfo()
{
	if (regular != nullptr)
	{
		TTF_CloseFont(regular);
		//delete regular;
	}

	if (bold != nullptr)
	{
		TTF_CloseFont(regular);
		//delete bold;
	}

	if (italics != nullptr)
	{
		TTF_CloseFont(regular);
		//delete italics;
	}

	if (boldItalics != nullptr)
	{
		TTF_CloseFont(regular);
		//delete boldItalics;
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

void FontInfo::SetRegularFont(const char* f)
{
	if (f != nullptr)
	{
		pathRegular = f;
		regular = TTF_OpenFont(f, size);
	}
}

void FontInfo::SetBoldFont(const char* f)
{
	pathBold = f;
	bold = TTF_OpenFont(f, size);
}

void FontInfo::SetItalicsFont(const char* f)
{
	pathItalics = f;
	italics = TTF_OpenFont(f, size);
}

void FontInfo::SetBoldItalicsFont(const char* f)
{
	pathBoldItalics = f;
	boldItalics = TTF_OpenFont(f, size);
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
