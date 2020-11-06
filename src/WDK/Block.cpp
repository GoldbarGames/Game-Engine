#include "Block.h"
#include "../ENGINE/Renderer.h"
#include "../ENGINE/Game.h"
#include "../ENGINE/Property.h"
#include "../ENGINE/PhysicsComponent.h"
#include "../ENGINE/HealthComponent.h"


Block::Block(const Vector2& pos) : MyEntity(pos)
{
	etype = "block";
	
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	impassable = true;
	trigger = true;
	
	physics = neww PhysicsComponent(this);
	physics->mass = 5;
	physics->canBePushed = true;
	physics->useGravity = true;
	physics->standAboveGround = true;
	physics->respawnOnDeath = true;
	physics->windResistance = 2.0f;
	physics->canBePickedUp = true;

	health = neww HealthComponent(1);
}

Block::~Block()
{

}

void Block::Init(const Game& g, const std::string& n)
{
	name = n;

	if (name == "big_block")
	{
		physics->mass = 10;
		CreateCollider(0, 16, 48, 48);
	}
	else if (name == "small_block")
	{
		physics->mass = 5;
		CreateCollider(0, 8, 24, 24);
	}
}

void Block::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Block::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);
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

void Block::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
}