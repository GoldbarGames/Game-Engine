#include "Block.h"
#include "Renderer.h"
#include "Game.h"

Block::Block(Vector2 pos) : Entity(pos)
{
	etype = "block";
	CreateCollider(0, -16, 48, 48);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	impassable = true;
	
	physics = new PhysicsEntity(this);
	physics->mass = 5;
	physics->canBePushed = true;
	physics->useGravity = true;
}

Block::~Block()
{

}

void Block::Render(Renderer * renderer)
{
	Entity::Render(renderer);
}

void Block::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(renderer, font, properties);

	properties.emplace_back(new Property(new Text(renderer, font, "Collider Pos X: " + std::to_string((int)colliderOffset.x))));
	properties.emplace_back(new Property(new Text(renderer, font, "Collider Pos Y: " + std::to_string((int)colliderOffset.y))));
	//properties.emplace_back(new Property(new Text(renderer, font, "Collider Width: " + std::to_string(colliderWidth))));
	//properties.emplace_back(new Property(new Text(renderer, font, "Collider Height: " + std::to_string(colliderHeight))));
}

void Block::SetProperty(std::string prop, std::string newValue)
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
	if (key == "Collider Pos X")
	{
		if (newValue != "")
			colliderOffset.x = std::stoi(newValue);
	}
	else if (key == "Collider Pos Y")
	{
		if (newValue != "")
			colliderOffset.y = std::stoi(newValue);
	}
	else if (key == "Collider Width")
	{
		//if (newValue != "")
		//	colliderWidth = std::stof(newValue);
	}
	else if (key == "Collider Height")
	{
		//if (newValue != "")
		//	colliderHeight = std::stof(newValue);
	}
}

void Block::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x <<
		" " << physics->startPosition.y << " " << spriteIndex << std::endl;
}