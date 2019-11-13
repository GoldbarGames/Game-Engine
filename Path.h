#pragma once
#include "PathNode.h"
#include "Entity.h"
class Path : public Entity
{
	// TODO: Move nodes around, insert into the middle, and insert onto the end of an existing path

public:
	bool shouldLoop = false;
	std::vector<PathNode*> nodes;
	Path(Vector2 startPoint);
	~Path();
	void AddPointToPath(Vector2 point);
	void RemovePointFromPath(Vector2 point);	
	bool IsPointInPath(Vector2 point);

	void Render(Renderer * renderer, Vector2 cameraOffset);
	const SDL_Rect* GetBounds();

	void Save(std::ostringstream& level);

	void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties);
	void SetProperty(std::string prop, std::string newValue);
};
