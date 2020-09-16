#ifndef PATH_H
#define PATH_H
#pragma once

#include "PathNode.h"
#include "Entity.h"

class Path : public Entity
{
	// TODO: Move nodes around, insert into the middle, and insert onto the end of an existing path

public:
	bool shouldLoop = false;
	std::vector<PathNode*> nodes;
	Path(const Vector2& startPoint);
	~Path();
	void AddPointToPath(const Vector2& point);
	void RemovePointFromPath(const Vector2& point);	
	bool IsPointInPath(const Vector2& point);

	void Render(const Renderer& renderer, unsigned int uniformModel);
	const SDL_Rect* GetBounds();

	void Save(std::ostringstream& level);
	void Load(int& index, const std::vector<std::string>& tokens,
		std::unordered_map<std::string, std::string>& map, Game& game);

	void GetProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue);
};

#endif