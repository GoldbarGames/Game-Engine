#include "Path.h"
#include "Renderer.h"
#include "Game.h"
#include "Property.h"
#include "Editor.h"
#include "Sprite.h"
#include "../WDK/MyEditorHelper.h"

Path::Path(const Vector2& pos) : Entity(pos)
{
	etype = "path";
	CreateCollider(0, 0, 24, 24);
	shouldSave = true;
}

Path::~Path()
{

}

void Path::AddPointToPath(const Vector2& point)
{
	//nodes.push_back(new PathNode(point));
}

void Path::RemovePointFromPath(const Vector2& point)
{
	int index = -1;
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		if (nodes[i]->position == point)
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
		if (nodes[i]->position == point)
			return true;
	}

	return false;
}

void Path::Update(Game& game)
{
	// We have nodes in the path, find them
	if (nodeCount > 0 && nodes.size() < nodeCount)
	{
		nodes.clear();
		int foundCount = 0;
		for (int k = 0; k < nodeCount; k++)
		{
			int nodeID = nodeIDs[k];
			for (int i = 0; i < game.entities.size(); i++)
			{
				if (game.entities[i]->etype == "pathnode")
				{
					int test = game.entities[i]->id;
					if (game.entities[i]->id == nodeID)
					{
						PathNode* pnode = static_cast<PathNode*>(game.entities[i]);
						nodes.push_back(pnode);
						foundCount++;
						break;
					}
				}
			}
		}

		if (foundCount < nodeCount)
		{
			nodeCount = 0;
		}
	}

	
}

void Path::Render(const Renderer& renderer)
{
	// Draw the path object itself
	renderer.debugSprite->color = {255, 255, 255, 255 };
	renderer.debugSprite->Render(position, renderer, scale);

	// Draw a red square for every point in the path
	
	for (unsigned int i = 0; i < nodes.size(); i++)
	{
		// Draw a red square in the center of the point
		//const SDL_Rect* pointRect = nodes[i]->CalcRenderRect(Vector2(0,0));

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

		//const SDL_Rect* nextRect = nodes[nextIndex]->CalcRenderRect(Vector2(0,0));
		//SDL_SetRenderDrawColor(renderer->renderer, 255, 255, 255, 255);
		//SDL_RenderDrawLine(renderer->renderer, pointRect->x, pointRect->y, nextRect->x, nextRect->y);
	}
}

void Path::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	// TODO: Maybe move this property to the thing on the path, not the path itself?
	std::string loopString = shouldLoop ? "True" : "False";
	properties.emplace_back(new Property("Should Loop", loopString));

	properties.emplace_back(new Property("Node Count", nodeCount));

	if (nodeCount > 0)
	{
		for (int i = 0; i < nodeCount; i++)
		{
			properties.emplace_back(new Property("Node " + std::to_string(i) + " ID", nodeIDs[i]));
		}
	}
}

void Path::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
	if (key == "Should Loop") //TODO: Maybe change this to a more general "end behavior"
	{
		shouldLoop = (newValue == "True");
	}
	else if (key == "Node Count")
	{
		if (newValue != "")
			nodeCount = std::stoi(newValue);
	}
	else if (newValue != "") // set ID for each node in the path
	{
		for (int i = 0; i < nodeCount; i++)
		{
			if (key == "Node " + std::to_string(i) + " ID")
			{
				nodeIDs[i] = std::stoi(newValue);
			}
		}
	}
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
		map["nodeX" + i] = std::to_string(nodes[i]->position.x);
		map["nodeY" + i] = std::to_string(nodes[i]->position.y);
	}
}

void Path::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	shouldLoop = std::stoi(map["shouldLoop"]);
	nodeCount = std::stoi(map["nodeCount"]);

	/*
	for (int i = 0; i < nodeCount; i++)
	{
		int pointX = std::stoi(map["pointX"]);
		int pointY = std::stoi(map["pointY"]);
		AddPointToPath(Vector2(pointX, pointY));
	}*/

	if (game.editor->helper != nullptr)
	{
		MyEditorHelper* helper = static_cast<MyEditorHelper*>(game.editor->helper);
		helper->loadListPaths.emplace_back(this);
	}

}