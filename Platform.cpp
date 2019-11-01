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
	mass = 10;
}


Platform::~Platform()
{

}

std::string Platform::CalcDirection(bool x)
{
	Vector2 nextPos = position + velocity;

	if (x)
	{
		// if we need to go to the right...
		if (nextPos.x < currentPath->nodes[pathNodeID]->point.x)
		{
			return "right";
		}
		// if we need to go to the left...
		else if (nextPos.x > currentPath->nodes[pathNodeID]->point.x)
		{
			return "left";
		}
		else // not moving horizontally at all
		{
			return "none";
		}
	}
	else
	{
		// if we need to go down...
		if (nextPos.y < currentPath->nodes[pathNodeID]->point.y)
		{
			return "down";
		}
		// if we need to go up...
		else if (nextPos.y > currentPath->nodes[pathNodeID]->point.y)
		{
			return "up";
		}
		else // not moving vertically at all
		{
			return "none";
		}
	}
}

void Platform::Update(Game& game)
{
	if (platformType == "Move")
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

			if (velocity.y > 0 && position.y >= startPosition.y + distance
				|| velocity.y < 0 && position.y <= startPosition.y - distance)
			{
				//TODO: Add a delay between moving the opposite direction
				velocity.y *= -1;
			}
		}
	}
	else if (platformType == "Path" && currentPath != nullptr && pathNodeID < currentPath->nodes.size() - 1)
	{
		// Move towards the next point in the path at the specified speed
		float dx = currentPath->nodes[pathNodeID]->point.x - position.x;
		float dy = currentPath->nodes[pathNodeID]->point.y - position.y;

		float length = sqrtf(dx*dx + dy * dy);

		// Normalize the vector
		dx /= length;
		dy /= length;

		dx *= pathSpeed;
		dy *= pathSpeed;

		velocity.x = dx;
		velocity.y = dy;


		std::string currentDirectionX = CalcDirection(true);
		std::string currentDirectionY = CalcDirection(false);

		bool wrongDirection = false;
		if (currentDirectionX != directionX || currentDirectionY != directionY)
		{
			wrongDirection = true;
		}

		// If we are at the point, then set the destination to the next point

		if (wrongDirection || position.RoundToInt() == currentPath->nodes[pathNodeID]->point.RoundToInt())
		{
			velocity.x = 0;
			velocity.y = 0;

			if (traversePathForward)
				pathNodeID++;
			else
				pathNodeID--;

			if (pathNodeID < currentPath->nodes.size() - 1)
			{
				directionX = CalcDirection(true);
				directionY = CalcDirection(false);
			}
			else // we have reached the end, so carry out end behavior
			{
				if (endPathBehavior == "Stop")
				{
					// Do nothing, because this is the default behavior!
				}
				else if (endPathBehavior == "Reverse")
				{
					// Reverse the direction of our index
					traversePathForward = !traversePathForward;
					if (traversePathForward)
						pathNodeID++;
					else
						pathNodeID--;
				}
				else if (endPathBehavior == "Selfdestruct")
				{
					// Destroy this entity
					shouldDelete = true;
				}
			}
		}
	}
	else
	{
		velocity.x = 0;
		velocity.y = 0;
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
	
	const std::vector<std::string> platformTypes = { "Idle", "Move", "Path" };
	properties.emplace_back(new Property(new Text(renderer, font, "Platform Type: " + platformType), platformTypes));

	if (platformType == "Move" || platformType == "Move")
	{
		properties.emplace_back(new Property(new Text(renderer, font, "Velocity X: " + std::to_string(startVelocity.x))));
		properties.emplace_back(new Property(new Text(renderer, font, "Velocity Y: " + std::to_string(startVelocity.y))));
		properties.emplace_back(new Property(new Text(renderer, font, "Distance: " + std::to_string(tilesToMove))));
		properties.emplace_back(new Property(new Text(renderer, font, "Loop: " + std::to_string(shouldLoop))));
	}
	else if (platformType == "Path")
	{
		const std::vector<std::string> behaviorOptions = { "Stop", "Reverse", "Selfdestruct" };
		properties.emplace_back(new Property(new Text(renderer, font, "Path ID: " + std::to_string(pathID))));
		properties.emplace_back(new Property(new Text(renderer, font, "Speed: " + std::to_string(pathSpeed))));
		properties.emplace_back(new Property(new Text(renderer, font, "End Behavior: " + endPathBehavior), behaviorOptions));
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

	if (platformType == "Path")
	{
		std::string endBehavior = endPathBehavior;
		if (endBehavior == "")
			endBehavior = "None";

		level << std::to_string(id) << " " << etype << " " << (startPosition.x / SCALE) << " " << (startPosition.y / SCALE) << " " << spriteIndex << " " << platformType
			<< " " << pathID << " " << pathSpeed << " " << endBehavior << std::endl;
	}
	else
	{
		level << std::to_string(id) << " " << etype << " " << (startPosition.x / SCALE) << " " << (startPosition.y / SCALE) << " " << spriteIndex << " " << platformType
			<< " " << startVelocity.x << " " << startVelocity.y << " " << tilesToMove << " " << shouldLoop << std::endl;
	}
	
}