#include "Block.h"
#include "Renderer.h"
#include "Game.h"
#include "PhysicsComponent.h"

Block::Block(const Vector2& pos) : Entity(pos)
{
	etype = "block";
	CreateCollider(0, -16, 48, 48);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	impassable = true;
	
	physics = new PhysicsComponent(this);
	physics->mass = 5;
	physics->canBePushed = true;
	physics->useGravity = true;
}

Block::~Block()
{

}

void Block::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Block::GetProperties(FontInfo * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(font, properties);

	properties.emplace_back(new Property(new Text(font, "Collider Pos X: " + std::to_string((int)collider->offset.x))));
	properties.emplace_back(new Property(new Text(font, "Collider Pos Y: " + std::to_string((int)collider->offset.y))));
	//properties.emplace_back(new Property(new Text(renderer, font, "Collider Width: " + std::to_string(colliderWidth))));
	//properties.emplace_back(new Property(new Text(renderer, font, "Collider Height: " + std::to_string(colliderHeight))));
}

void Block::SetProperty(const std::string& prop, const std::string& newValue)
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
	//TODO: Make this more robust
	if (key == "Collider Pos X")
	{
		if (newValue != "")
			collider->offset.x = std::stoi(newValue);
	}
	else if (key == "Collider Pos Y")
	{
		if (newValue != "")
			collider->offset.y = std::stoi(newValue);
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