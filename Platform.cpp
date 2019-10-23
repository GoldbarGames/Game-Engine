#include "Platform.h"
#include "Renderer.h"
#include "Game.h"

Platform::Platform(Vector2 pos) : PhysicsEntity(pos)
{
	startPosition = position;
	etype = "platform";
	CreateCollider(0, 0, 0, 0, 72, 24);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;
	canBePushed = true;
	impassable = true;
	useGravity = false;
}


Platform::~Platform()
{

}

void Platform::Update(Game& game)
{
	if (platformType == "Move-Horizontal")
	{
		if (shouldLoop)
		{
			int distance = (tilesToMove * TILE_SIZE * Renderer::GetScale());
			if (velocity.x > 0 && position.x >= startPosition.x + distance
				|| velocity.x < 0 && position.x <= startPosition.x - distance)
			{
				//TODO: Add a delay between moving the opposite direction
				velocity.x *= -1;
			}
		}
		
	}
	else if (platformType == "Move-Vertical")
	{
		if (shouldLoop)
		{
			int distance = (tilesToMove * TILE_SIZE * Renderer::GetScale());
			if (velocity.y > 0 && position.y >= startPosition.y + distance
				|| velocity.y < 0 && position.y <= startPosition.y - distance)
			{
				//TODO: Add a delay between moving the opposite direction
				velocity.y *= -1;
			}
		}
	}

	

	PhysicsEntity::Update(game);
}

void Platform::Render(Renderer * renderer, Vector2 cameraOffset)
{
	PhysicsEntity::Render(renderer, cameraOffset);
}

void Platform::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Text*>& properties)
{
	Entity::DeleteProperties(properties);
	
	properties.emplace_back(new Text(renderer, font, "Platform Type: " + platformType));

	if (platformType == "Move-Horizontal")
	{
		properties.emplace_back(new Text(renderer, font, "Velocity X: " + std::to_string(startVelocity.x)));
		properties.emplace_back(new Text(renderer, font, "Distance: " + std::to_string(tilesToMove)));
		properties.emplace_back(new Text(renderer, font, "Loop: " + std::to_string(shouldLoop)));
	}
	else if (platformType == "Move-Vertical")
	{
		properties.emplace_back(new Text(renderer, font, "Velocity Y: " + std::to_string(startVelocity.y)));
		properties.emplace_back(new Text(renderer, font, "Distance: " + std::to_string(tilesToMove)));
		properties.emplace_back(new Text(renderer, font, "Loop: " + std::to_string(shouldLoop)));
	}
	else if (platformType == "Idle")
	{

	}

	//properties.emplace_back(new Text(renderer, font, "Collider Pos X: " + std::to_string((int)collider->x)));
	//properties.emplace_back(new Text(renderer, font, "Collider Pos Y: " + std::to_string((int)collider->y)));
	//properties.emplace_back(new Text(renderer, font, "Collider Width: " + std::to_string(colliderWidth)));
	//properties.emplace_back(new Text(renderer, font, "Collider Height: " + std::to_string(colliderHeight)));
}

void Platform::SetProperty(std::string prop, std::string newValue)
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
	if (key == "Velocity X")
	{
		if (newValue != "")
			startVelocity.x = std::stof(newValue);
	}
	else if (key == "Velocity Y")
	{
		if (newValue != "")
			startVelocity.y = std::stof(newValue);
	}
	else if (key == "Distance")
	{
		if (newValue != "")
			tilesToMove = std::stoi(newValue) * Renderer::GetScale();
	}
	else if (key == "Loop")
	{
		if (newValue != "")
			shouldLoop = (newValue == "True" ) ? true : false;
	}
	else if (key == "Platform Type")
	{
		if (newValue != "")
			platformType = newValue;
	}
}

void Platform::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();
	Vector2 pos = GetPosition();

	level << etype << " " << (pos.x / SCALE) << " " << (pos.y / SCALE) << " " << spriteIndex <<	" " << platformType
	<< " " << startVelocity.x << " " << startVelocity.y << " " << tilesToMove << " " << shouldLoop << std::endl;
}