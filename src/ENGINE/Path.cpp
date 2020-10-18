#include "Path.h"
#include "Renderer.h"
#include "Game.h"
#include "Property.h"
#include "Editor.h"

Path::Path(const Vector2& pos) : Entity(pos)
{
	etype = "path";
}

Path::~Path()
{

}

void Path::AddPointToPath(const Vector2& point)
{
	nodes.push_back(new PathNode(point));
}

void Path::RemovePointFromPath(const Vector2& point)
{
	int index = -1;
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		if (nodes[i]->point == point)
			index = i;
	}

	// If the point exists in the list, then remove it
	if (index > -1)
	{
		delete nodes[index];
		nodes.erase(nodes.begin() + index);
	}		
}

bool Path::IsPointInPath(const Vector2& point)
{
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		if (nodes[i]->point == point)
			return true;
	}

	return false;
}

void Path::Render(const Renderer& renderer, unsigned int uniformModel)
{
	// Draw a red square for every point in the path
	
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		// Draw a red square in the center of the point
		const SDL_Rect* pointRect = nodes[i]->CalcRenderRect(Vector2(0,0));

		// Only show the points in the editor
		if (renderer.game->editMode)
		{		
			/*
			if (i == 0)
				SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
			else if (i == nodes.size() - 1)
				SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 255, 255);
			else
				SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 255);

			SDL_RenderFillRect(renderer->renderer, pointRect);
			*/
		}

		// Draw a white line connecting to the next point
		int nextIndex = i + 1;
		if (nextIndex >= nodes.size())
		{
			if (shouldLoop)
				nextIndex = 0;
			else
				continue;
		}

		const SDL_Rect* nextRect = nodes[nextIndex]->CalcRenderRect(Vector2(0,0));
		//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		//SDL_RenderDrawLine(renderer->renderer, pointRect->x, pointRect->y, nextRect->x, nextRect->y);
	}
}

void Path::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	std::string loopString = shouldLoop ? "True" : "False";
	properties.emplace_back(new Property("Should Loop", name));
}

void Path::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
	if (key == "Should Loop") //TODO: Maybe change this to a more general "end behavior"
	{
		shouldLoop = (newValue == "True");
	}
}

const SDL_Rect* Path::GetBounds()
{
	return nodes[0]->GetRenderRect();
}

void Path::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["shouldLoop"] = std::to_string(shouldLoop);
	map["nodeCount"] = std::to_string(nodes.size());

	//TODO: Handle this as a special case in the main Save() function
	for (int i = 0; i < nodes.size(); i++)
	{
		map["nodeX" + i] = std::to_string(nodes[i]->point.x);
		map["nodeY" + i] = std::to_string(nodes[i]->point.y);
	}
}

void Path::Load(int& index, const std::vector<std::string>& tokens,
	std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	shouldLoop = std::stoi(map["shouldLoop"]);
	int nodeCount = std::stoi(map["nodeCount"]);

	for (int i = 0; i < nodeCount; i++)
	{
		int pointX = std::stoi(map["pointX"]);
		int pointY = std::stoi(map["pointY"]);
		AddPointToPath(Vector2(pointX, pointY));
	}

	game.editor->loadListPaths.emplace_back(this);
}