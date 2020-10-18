#ifndef FONTINFO_H
#define FONTINFO_H
#pragma once

#include <string>

typedef struct _TTF_Font TTF_Font;

class FontInfo
{
private:
	int size = 1;
	TTF_Font* regular = nullptr;
	TTF_Font* bold = nullptr;
	TTF_Font* italics = nullptr;
	TTF_Font* boldItalics = nullptr;
	std::string pathRegular = "";
	std::string pathBold = "";
	std::string pathItalics = "";
	std::string pathBoldItalics = "";

public:

	FontInfo(const char* f, int s);

	~FontInfo();

	const int GetFontSize();

	void ChangeFontSize(int newSize);

	void SetRegularFont(const char* f);

	void SetBoldFont(const char* f);

	void SetItalicsFont(const char* f);

	void SetBoldItalicsFont(const char* f);

	TTF_Font* GetRegularFont();

	TTF_Font* GetBoldFont();

	TTF_Font* GetItalicsFont();

	TTF_Font* GetBoldItalicsFont();


};

#endif