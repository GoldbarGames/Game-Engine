#ifndef PROPERTY_H
#define PROPERY_H
#pragma once

#include <vector>
#include <string>
#include "leak_check.h"

class Text;
class FontInfo;

enum class PropertyType { String, Integer, Float, ReadOnly };

class DECLSPEC Property
{
public:
	static FontInfo* fontInfo;
	Text* text = nullptr;
	std::vector<std::string> options;
	std::string key = "";
	std::string value = "";
	PropertyType pType;
	Property(const std::string& k, const std::string& v, const std::vector<std::string>& o = std::vector<std::string>());
	Property(const std::string& k, const float v, const std::vector<std::string>& o = std::vector<std::string>());
	Property(const std::string& k, const int v, const std::vector<std::string>& o = std::vector<std::string>());
	Property(const std::string& k, const uint32_t v, const std::vector<std::string>& o = std::vector<std::string>());
	~Property();
};

#endif