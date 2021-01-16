#include "Property.h"
#include "FontInfo.h"
#include "Text.h"
#include "Editor.h"

// TODO: Refactor class so that we don't store a Text in every Property, it's wasteful.
// Rather, the editor should have a set number of texts.

Property::Property(const std::string& k, std::string& v, const std::vector<std::string>& o) 
	: key(k), value(v), pType(PropertyType::String)
{
	pString = &v;
	text = new Text(Editor::fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, int& v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Integer)
{
	pInt = &v;
	text = new Text(Editor::fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, float& v, const std::vector<std::string>& o) 
	: key(k), value(std::to_string(v)), pType(PropertyType::Float)
{
	pFloat = &v;
	text = new Text(Editor::fontInfo, key + ": " + value);
}

Property::Property(const std::string& k, uint32_t v, const std::vector<std::string>& o)
	: key(k), value(std::to_string(v)), pType(PropertyType::ReadOnly)
{
	text = new Text(Editor::fontInfo, key + ": " + value);
}

void Property::SetProperty(const std::string& value)
{
	if (value == "")
		return;

	try
	{
		if (pType == PropertyType::Integer && pInt != nullptr)
		{
			*pInt = std::stoi(value);
		}
		else if (pType == PropertyType::Float && pFloat != nullptr)
		{
			*pFloat = std::stof(value);
		}
		else if (pType == PropertyType::String && pString != nullptr)
		{
			*pString = value;
		}
		else
		{
			std::cout << "ERROR: Failed to set property " << key << " with value " << value << std::endl;
		}
	}
	catch (std::exception ex)
	{
		std::cout << "ERROR: Failed to set property " << key << " with value " << value << ": " << ex.what() << std::endl;
	}


}

Property::~Property()
{
	if (text != nullptr)
		delete_it(text);
}
