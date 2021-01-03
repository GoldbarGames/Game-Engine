#ifndef PATH_H
#define PATH_H
#pragma once

#include "PathNode.h"
#include "Entity.h"
#include "leak_check.h"
class KINJO_API Path : public Entity
{
	// TODO: Move nodes around, insert into the middle, and insert onto the end of an existing path

public:
	bool shouldLoop = false;
	int nodeCount = 0;
	std::vector<PathNode*> nodes;
	std::unordered_map<int, int> nodeIDs;


	Path(const glm::vec3& startPoint);
	~Path();
	void AddPointToPath(const glm::vec3& point);
	void RemovePointFromPath(const glm::vec3& point);
	bool IsPointInPath(const glm::vec3& point);

	void Update(Game& game);
	void Render(const Renderer& renderer);

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);

	static Entity* __stdcall Create(const glm::vec3& pos) { return neww Path(pos); };
};

#endif