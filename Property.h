#ifndef PROPERTY_H
#define PROPERY_H
#pragma once

#include <vector>
#include <string>

class Text;

class Property
{
public:
	Text* text;
	std::vector<std::string> options;

	Property(Text* t, const std::vector<std::string>& o = std::vector<std::string>());
	~Property();
};

#endif