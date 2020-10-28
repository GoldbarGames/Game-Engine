#include "PathNode.h"
#include "Renderer.h"
#include "Game.h"

PathNode::PathNode(const Vector2& pos) : Entity(pos)
{
	etype = "pathnode";
	shouldSave = true;
}

PathNode::~PathNode()
{

}

void PathNode::Render(const Renderer& renderer)
{
	if (renderer.game->editMode || renderer.game->debugMode)
	{
		renderer.debugSprite->color = { 0, 255, 255, 255 };
		renderer.debugSprite->Render(position, renderer, scale);
	}
}

void PathNode::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);
}

void PathNode::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);
}

void PathNode::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);
}

void PathNode::SetProperty(const std::string& key, const std::string& newValue)
{

}