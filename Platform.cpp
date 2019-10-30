#include "Platform.h"
#include "Renderer.h"
#include "Game.h"

// In order to make a platform move along a path:

// * 0a. Display ID as read-only property for every entity
// * we need to save IDs for every entity
// * we need to calculate the next ID when we load the level (by finding the highest one)

// 0b. Create a Path class and a way to spawn Path objects in the level using the editor
// * Click a button to start drawing a path. Each left-click will place a path node on a tile.
// * Right-clicking will remove nodes from the path
// * Clicking on the path button will save the path
// * Can toggle the property of the path to make it a closed loop or not
// ? Can you move nodes in the path? Can you insert nodes into the middle?

// *1. Click on platform, set its type property to Move-Path (select via up/down arrows)
// *2. After its type is set, properties for speed and Path ID will be displayed
// *3. Make the platform save its path ID when we save the platform to the level (and save the path itself too)
// *4. When the level starts, make the platform get the path from the ID
// 5. Keep track of which node the platform is heading towards, and move toward it
// 6. Define behavior when the end of the path is reached, and take that action


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

void Platform::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(renderer, font, properties);
	
	const std::vector<std::string> platformTypes = { "Idle", "Move-Horizontal", "Move-Vertical", "Move-Path" };
	properties.emplace_back(new Property(new Text(renderer, font, "Platform Type: " + platformType), platformTypes));

	if (platformType == "Move-Horizontal")
	{
		properties.emplace_back(new Property(new Text(renderer, font, "Velocity X: " + std::to_string(startVelocity.x))));
		properties.emplace_back(new Property(new Text(renderer, font, "Distance: " + std::to_string(tilesToMove))));
		properties.emplace_back(new Property(new Text(renderer, font, "Loop: " + std::to_string(shouldLoop))));
	}
	else if (platformType == "Move-Vertical")
	{
		properties.emplace_back(new Property(new Text(renderer, font, "Velocity Y: " + std::to_string(startVelocity.y))));
		properties.emplace_back(new Property(new Text(renderer, font, "Distance: " + std::to_string(tilesToMove))));
		properties.emplace_back(new Property(new Text(renderer, font, "Loop: " + std::to_string(shouldLoop))));
	}
	else if (platformType == "Move-Path")
	{
		properties.emplace_back(new Property(new Text(renderer, font, "Path ID: " + std::to_string(pathID))));
		properties.emplace_back(new Property(new Text(renderer, font, "Speed: " + std::to_string(pathSpeed))));
		properties.emplace_back(new Property(new Text(renderer, font, "End Behavior: " + endPathBehavior)));
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
		shouldLoop = (newValue == "True" ) ? true : false;
	}
	else if (key == "Platform Type")
	{
		platformType = newValue;
	}
	else if (key == "Path ID")
	{
		if (newValue != "")
			pathID = std::stoi(newValue);
	}
	else if (key == "Speed")
	{
		if (newValue != "")
			pathSpeed = std::stof(newValue);
	}
	else if (key == "End Behavior")
	{
		if (newValue != "")
			endPathBehavior = newValue;
	}
}

void Platform::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();
	Vector2 pos = GetPosition();

	if (platformType == "Move-Path")
	{
		std::string endBehavior = endPathBehavior;
		if (endBehavior == "")
			endBehavior = "None";

		level << std::to_string(id) << " " << etype << " " << (pos.x / SCALE) << " " << (pos.y / SCALE) << " " << spriteIndex << " " << platformType
			<< " " << pathID << " " << pathSpeed << " " << endBehavior << std::endl;
	}
	else
	{
		level << std::to_string(id) << " " << etype << " " << (pos.x / SCALE) << " " << (pos.y / SCALE) << " " << spriteIndex << " " << platformType
			<< " " << startVelocity.x << " " << startVelocity.y << " " << tilesToMove << " " << shouldLoop << std::endl;
	}
	
}