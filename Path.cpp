#include "Path.h"
#include "Renderer.h"
#include "Game.h"

Path::Path(Vector2 pos) : Entity(pos)
{
	etype = "path";
}

Path::~Path()
{

}

void Path::AddPointToPath(Vector2 point)
{
	nodes.push_back(new PathNode(point));
}

void Path::RemovePointFromPath(Vector2 point)
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

bool Path::IsPointInPath(Vector2 point)
{
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		if (nodes[i]->point == point)
			return true;
	}

	return false;
}

void Path::Render(Renderer * renderer, Vector2 cameraOffset)
{
	// Draw a red square for every point in the path
	
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		// Draw a red square in the center of the point
		const SDL_Rect* pointRect = nodes[i]->CalcRenderRect(cameraOffset);

		// Only show the points in the editor
		if (GetModeEdit())
		{		
			if (i == 0)
				SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 0, 255);
			else if (i == nodes.size() - 1)
				SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 255, 255);
			else
				SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 255);

			SDL_RenderFillRect(renderer->renderer, pointRect);
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

		const SDL_Rect* nextRect = nodes[nextIndex]->CalcRenderRect(cameraOffset);
		SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		SDL_RenderDrawLine(renderer->renderer, pointRect->x, pointRect->y, nextRect->x, nextRect->y);
	}
}

void Path::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(renderer, font, properties);

	std::string loopString = shouldLoop ? "True" : "False";
	properties.emplace_back(new Property(new Text(renderer, font, "Should Loop: " + name)));
}

void Path::SetProperty(std::string prop, std::string newValue)
{
	// 1. Split the string into two (key and value)
	std::string key = "";

	int index = 0;
	while (prop[index] != ':')
	{
		key += prop[index];
		index++;
	}

	// 2. Based on the key, change its value
	if (key == "Should Loop") //TODO: Maybe change this to a more general "end behavior"
	{
		shouldLoop = (newValue == "True");
	}
}

void Path::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();
	Vector2 pos = GetPosition();

	level << std::to_string(id) << " " << etype << " " << (pos.x / SCALE) << " " <<
		(pos.y / SCALE) << " " << shouldLoop << " " << nodes.size();

	for (int i = 0; i < nodes.size(); i++)
	{
		level << " " << nodes[i]->point.x << " " << nodes[i]->point.y;
	}

	level << std::endl;		
}

const SDL_Rect* Path::GetBounds()
{
	return nodes[0]->GetRenderRect();
}