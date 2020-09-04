#include "Block.h"
#include "Renderer.h"
#include "Game.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"

Block::Block(const Vector2& pos) : Entity(pos)
{
	etype = "block";
	CreateCollider(0, 16, 48, 48);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	impassable = true;
	
	physics = new PhysicsComponent(this);
	physics->mass = 5;
	physics->canBePushed = true;
	physics->useGravity = true;
	physics->standAboveGround = true;
	physics->respawnOnDeath = true;
	physics->windResistance = 2.0f;

	health = new HealthComponent(1);
}

Block::~Block()
{

}

void Block::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Block::GetProperties(FontInfo* font, std::vector<Property*>& properties)
{
	Entity::GetProperties(font, properties);

	properties.emplace_back(new Property("Collider Pos X", (int)collider->offset.x));
	properties.emplace_back(new Property("Collider Pos Y", (int)collider->offset.y));
	properties.emplace_back(new Property("Collider Width", collider->scale.x));
	properties.emplace_back(new Property("Collider Height", collider->scale.y));
}

void Block::SetProperty(const std::string& key, const std::string& newValue)
{	
	// Based on the key, change its value
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
		if (newValue != "")
			collider->scale.x = std::stof(newValue);
	}
	else if (key == "Collider Height")
	{
		if (newValue != "")
			collider->scale.y = std::stof(newValue);
	}
}

void Block::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x <<
		" " << physics->startPosition.y << " " << spriteIndex << std::endl;
}