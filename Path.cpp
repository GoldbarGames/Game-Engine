#include "Path.h"
#include "Renderer.h"
#include "Game.h"

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

void Path::Save(std::ostringstream& level)
{
	Vector2 pos = GetPosition();

	level << std::to_string(id) 
		<< " " << etype 
		<< " " << pos.x 
		<< " " << pos.y 
		<< " " << shouldLoop 
		<< " " << nodes.size();

	for (int i = 0; i < nodes.size(); i++)
	{
		level 
			<< " " << nodes[i]->point.x 
			<< " " << nodes[i]->point.y;
	}

	level << std::endl;		
}

void Path::Load(int& index, const std::vector<std::string>& tokens,
	std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(index, tokens, map, game);

	shouldLoop = std::stoi(tokens[index++]);
	int nodeCount = std::stoi(tokens[index++]);

	for (int i = 0; i < nodeCount; i++)
	{
		int pointX = std::stoi(tokens[index++]);
		int pointY = std::stoi(tokens[index++]);
		AddPointToPath(Vector2(pointX, pointY));
	}

	game.editor->loadListPaths.emplace_back(this);
}