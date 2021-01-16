#pragma once
#ifndef CAMERABOUNDS_H
#define CAMERABOUNDS_H
#include "leak_check.h"
#include "Entity.h"

class KINJO_API CameraBounds : public Entity
{
public:
	CameraBounds(glm::vec3 pos);
	~CameraBounds();
	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);
	void Render(const Renderer& renderer);

	static Entity* __stdcall Create(const glm::vec3& pos) { return new CameraBounds(pos); };
};



#endif