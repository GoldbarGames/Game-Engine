#include "Path.h"
#include "Renderer.h"
#include "Game.h"
#include "Property.h"
#include "Editor.h"
#include "Sprite.h"

Path::Path(const glm::vec3& pos) : Entity(pos)
{
	etype = "path";
	CreateCollider(0, 0, 24, 24);
	shouldSave = true;
}

Path::~Path()
{

}

void Path::AddPointToPath(const glm::vec3& point)
{
	//nodes.push_back(new PathNode(point));
}

void Path::RemovePointFromPath(const glm::vec3& point)
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

bool Path::IsPointInPath(const glm::vec3& point)
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
	if (renderer.game->editMode || renderer.game->debugMode)
	{
		// Draw the path object itself
		renderer.debugSprite->color = { 255, 255, 255, 255 };
		renderer.debugSprite->Render(position, renderer, scale);

		// TODO: Draw a line connecting each point in the path
	}
}

void Path::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	// TODO: Maybe move this property to the thing on the path, not the path itself?
	properties.emplace_back(new Property("Should Loop", shouldLoop));
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
				nodes.clear(); // clear the list to find the new object
			}
		}
	}
}

void Path::Save(std::unordered_map<std::string, std::string>& map)
{	
	Entity::Save(map);

	map["shouldLoop"] = std::to_string(shouldLoop);
	map["nodeCount"] = std::to_string(nodes.size());

	std::string key = "";

	for (int i = 0; i < nodes.size(); i++)
	{
		key = "nodeID_" + std::to_string(i);
		map[key] = std::to_string(nodes[i]->id);
	}
}

void Path::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	shouldLoop = std::stoi(map["shouldLoop"]);
	nodeCount = std::stoi(map["nodeCount"]);

	for (int i = 0; i < nodeCount; i++)
	{
		nodeIDs[i] = std::stoi(map["nodeID_" + std::to_string(i)]);
	}
}