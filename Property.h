#pragma once

#include <vector>
#include "Text.h"

class Property
{
public:
	Text* text;
	std::vector<std::string> options;

	Property(Text* t, const std::vector<std::string>& o = std::vector<std::string>());
	~Property();
};

