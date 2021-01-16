#ifndef PROPERTY_H
#define PROPERTY_H
#pragma once

#include <vector>
#include <string>
#include "leak_check.h"

class Text;

enum class PropertyType { String, Integer, Float, ReadOnly };

class KINJO_API Property
{
public:
	Text* text = nullptr;
	std::vector<std::string> options;
	std::string key = "";
	std::string value = "";
	
	// Pointers to the data to modify
	std::string* pString = nullptr;
	float* pFloat = nullptr;
	int* pInt = nullptr;

	PropertyType pType;
	Property(const std::string& k, std::string& v, const std::vector<std::string>& o = std::vector<std::string>());
	Property(const std::string& k, float& v, const std::vector<std::string>& o = std::vector<std::string>());
	Property(const std::string& k, int& v, const std::vector<std::string>& o = std::vector<std::string>());

	Property(const std::string& k, uint32_t v, const std::vector<std::string>& o = std::vector<std::string>());

	void SetProperty(const std::string& value);

	~Property();
};

#endif