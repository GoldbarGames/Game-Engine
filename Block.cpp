#include "Block.h"
#include "Renderer.h"
#include "Game.h"

Block::Block(Vector2 pos) : PhysicsEntity(pos)
{
	etype = "block";
	CreateCollider(0, 0, 0, -16, 48, 48);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	canBePushed = true;
	impassable = true;
	mass = 5;
}

Block::~Block()
{

}

void Block::Render(Renderer * renderer, GLuint uniformModel)
{
	PhysicsEntity::Render(renderer, uniformModel);
}

void Block::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(renderer, font, properties);

	properties.emplace_back(new Property(new Text(renderer, font, "Collider Pos X: " + std::to_string((int)collider->x))));
	properties.emplace_back(new Property(new Text(renderer, font, "Collider Pos Y: " + std::to_string((int)collider->y))));
	properties.emplace_back(new Property(new Text(renderer, font, "Collider Width: " + std::to_string(colliderWidth))));
	properties.emplace_back(new Property(new Text(renderer, font, "Collider Height: " + std::to_string(colliderHeight))));
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
			collider->x = std::stof(newValue);
	}
	else if (key == "Collider Pos Y")
	{
		if (newValue != "")
			collider->y = std::stof(newValue);
	}
	else if (key == "Collider Width")
	{
		if (newValue != "")
			colliderWidth = std::stof(newValue) * Renderer::GetScale();
	}
	else if (key == "Collider Height")
	{
		if (newValue != "")
			colliderHeight = std::stof(newValue) * Renderer::GetScale();
	}
}

void Block::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();
	
	level << std::to_string(id) << " " << etype << " " << (startPosition.x / SCALE) <<
		" " << (startPosition.y / SCALE) << " " << spriteIndex << std::endl;
}