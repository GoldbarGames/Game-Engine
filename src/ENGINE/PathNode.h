#ifndef PATHNODE_H
#define PATHNODE_H
#pragma once

#include "Entity.h"
#include "Vector2.h"
#include <SDL.h>
class PathNode : public Entity
{

public:
	PathNode(const Vector2& pos);
	~PathNode();

	void Render(const Renderer& renderer);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	static Entity* __stdcall Create(const Vector2& pos) { return neww PathNode(pos); };
};

#endif