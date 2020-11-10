#include "Property.h"
#include "FontInfo.h"
#include "Text.h"
#include "Editor.h"

//TODO: Refactor class so that we don't store a Text in every Property, it's wasteful
Property::Property(const std::string& k, const std::string& v, const std::vector<std::string>& o) 
	: key(k), value(v), pType(PropertyType::String)
{
	text = neww Text(Editor::fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const int v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Integer)
{
	// TODO: MEMORY LEAK
	text = neww Text(Editor::fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const uint32_t v, const std::vector<std::string>& o)
	: key(k), value(std::to_string(v)), pType(PropertyType::Integer)
{
	// TODO: MEMORY LEAK
	text = neww Text(Editor::fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, const float v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Float)
{
	text = neww Text(Editor::fontInfo, key + ": " + value);
}

Property::~Property()
{
	delete text;
}
