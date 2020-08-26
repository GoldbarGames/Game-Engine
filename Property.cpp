#include "Property.h"
#include "FontInfo.h"
#include "Text.h"

FontInfo* Property::fontInfo;

Property::Property(Text* t, const std::vector<std::string>& o) : text(t), options(o) { }

//TODO: Refactor class so that we don't store a Text in every Property, it's wasteful
Property::Property(const std::string& k, const std::string& v, const std::vector<std::string>& o) 
	: key(k), value(v), pType(PropertyType::String)
{
	if (fontInfo == nullptr)
	{
		fontInfo = new FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	text = new Text(fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const int v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Integer)
{
	if (fontInfo == nullptr)
	{
		fontInfo = new FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	text = new Text(fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const float v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Float)
{
	if (fontInfo == nullptr)
	{
		fontInfo = new FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	text = new Text(fontInfo, key + ": " + value);
}

Property::~Property()
{
	delete text;
}
