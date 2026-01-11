#ifndef PATHNODE_H
#define PATHNODE_H
#pragma once

#include "Entity.h"
#include <SDL2/SDL.h>
#include "leak_check.h"
class KINJO_API PathNode : public Entity
{

public:
	PathNode(const glm::vec3& pos);
	~PathNode();

	void Render(const Renderer& renderer);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	static Entity* __stdcall Create(const glm::vec3& pos) { return new PathNode(pos); };
};

#endif