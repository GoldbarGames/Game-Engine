#include "Property.h"
#include "FontInfo.h"
#include "Text.h"

FontInfo* Property::fontInfo;

//TODO: Refactor class so that we don't store a Text in every Property, it's wasteful
Property::Property(const std::string& k, const std::string& v, const std::vector<std::string>& o) 
	: key(k), value(v), pType(PropertyType::String)
{
	if (fontInfo == nullptr)
	{
		fontInfo = neww FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	text = neww Text(fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const int v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Integer)
{
	if (fontInfo == nullptr)
	{
		fontInfo = neww FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	// TODO: MEMORY LEAK
	text = neww Text(fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const uint32_t v, const std::vector<std::string>& o)
	: key(k), value(std::to_string(v)), pType(PropertyType::Integer)
{
	if (fontInfo == nullptr)
	{
		// TODO: MEMORY LEAK
		fontInfo = neww FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	// TODO: MEMORY LEAK
	text = neww Text(fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const float v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Float)
{
	if (fontInfo == nullptr)
	{
		fontInfo = neww FontInfo("fonts/space-mono/SpaceMono-Regular.ttf", 24);
		fontInfo->SetBoldFont("fonts/space-mono/SpaceMono-Bold.ttf");
		fontInfo->SetItalicsFont("fonts/space-mono/SpaceMono-Italic.ttf");
		fontInfo->SetBoldItalicsFont("fonts/space-mono/SpaceMono-BoldItalic.ttf");
	}

	text = neww Text(fontInfo, key + ": " + value);
}

Property::~Property()
{
	delete text;
}
