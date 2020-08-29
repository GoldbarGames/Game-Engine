#include "Platform.h"
#include "Renderer.h"
#include "Game.h"
#include "PhysicsComponent.h"

Platform::Platform(const Vector2& pos) : Entity(pos)
{
	startPosition = position;
	etype = "platform";
	CreateCollider(0, 24, 72, 24);
	layer = DrawingLayer::COLLISION;
	drawOrder = 9;
	
	jumpThru = true;
	impassable = true;

	physics = new PhysicsComponent(this);
	physics->canBePushed = false; // TODO: Is there some potential here?
	physics->useGravity = false;
	physics->mass = 10;
}


Platform::~Platform()
{

}

std::string Platform::CalcDirection(bool x)
{
	Vector2 nextPos = position + physics->velocity;

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
		physics->SetVelocity(startVelocity);

		if (shouldLoop)
		{
			int distance = (tilesToMove * TILE_SIZE);
			if (physics->velocity.x > 0 && position.x >= startPosition.x + distance
				|| physics->velocity.x < 0 && position.x <= startPosition.x - distance)
			{
				//TODO: Add a delay between moving the opposite direction
				startVelocity.x *= -1;
			}

			if (physics->velocity.y > 0 && position.y >= startPosition.y + distance
				|| physics->velocity.y < 0 && position.y <= startPosition.y - distance)
			{
				//TODO: Add a delay between moving the opposite direction
				startVelocity.y *= -1;
			}
		}
	}
	else if (platformType == "Path" && currentPath != nullptr && pathNodeID < currentPath->nodes.size() - 1)
	{
		//float posCenterX = position.x + (GetSprite()->frameWidth / 2);
		//float posCenterY = position.x + (GetSprite()->frameHeight / 2);
		//Vector2 posCenter = Vector2(posCenterX, posCenterY);

		// Move towards the next point in the path at the specified speed
		float dx = currentPath->nodes[pathNodeID]->point.x - position.x;
		float dy = currentPath->nodes[pathNodeID]->point.y - position.y;

		float length = sqrtf(dx*dx + dy*dy);

		// Normalize the vector
		dx /= length;
		dy /= length;

		dx *= pathSpeed;
		dy *= pathSpeed;

		physics->velocity.x = dx;
		physics->velocity.y = dy;

		std::string currentDirectionX = CalcDirection(true);
		std::string currentDirectionY = CalcDirection(false);

		bool wrongDirection = false;
		if (currentDirectionX != directionX || currentDirectionY != directionY)
		{
			wrongDirection = true;
		}

		// If we are at the point, then set the destination to the next point

		if (wrongDirection || RoundToInt(position) == RoundToInt(currentPath->nodes[pathNodeID]->point))
		{
			physics->velocity.x = 0;
			physics->velocity.y = 0;

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
				else if (endPathBehavior == "Fall")
				{
					// Detach from the path and fall downward due to gravity
					physics->useGravity = true;
					startVelocity = Vector2(0, 0);
					physics->velocity = startVelocity;
					platformType = "";
				}
			}
		}
	}
	else if (platformType == "Idle")
	{
		physics->velocity.x = 0;
		physics->velocity.y = 0;
	}

	

	Entity::Update(game);


}

void Platform::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Platform::GetProperties(FontInfo* font, std::vector<Property*>& properties)
{
	Entity::GetProperties(font, properties);
	
	const std::vector<std::string> platformTypes = { "Idle", "Move", "Path" };
	properties.emplace_back(new Property(new Text(font, "Platform Type: " + platformType), platformTypes));

	if (platformType == "Move" || platformType == "Move")
	{
		properties.emplace_back(new Property(new Text(font, "Velocity X: " + std::to_string(startVelocity.x))));
		properties.emplace_back(new Property(new Text(font, "Velocity Y: " + std::to_string(startVelocity.y))));
		properties.emplace_back(new Property(new Text(font, "Distance: " + std::to_string(tilesToMove))));
		properties.emplace_back(new Property(new Text(font, "Loop: " + std::to_string(shouldLoop))));
	}
	else if (platformType == "Path")
	{
		const std::vector<std::string> behaviorOptions = { "Stop", "Reverse", "Selfdestruct", "Fall" };
		properties.emplace_back(new Property(new Text(font, "Path ID: " + std::to_string(pathID))));
		properties.emplace_back(new Property(new Text(font, "Speed: " + std::to_string(pathSpeed))));
		properties.emplace_back(new Property(new Text(font, "End Behavior: " + endPathBehavior), behaviorOptions));
	}
	else if (platformType == "Idle")
	{

	}

	//properties.emplace_back(new Text(renderer, font, "Collider Pos X: " + std::to_string((int)collider->x)));
	//properties.emplace_back(new Text(renderer, font, "Collider Pos Y: " + std::to_string((int)collider->y)));
	//properties.emplace_back(new Text(renderer, font, "Collider Width: " + std::to_string(colliderWidth)));
	//properties.emplace_back(new Text(renderer, font, "Collider Height: " + std::to_string(colliderHeight)));
}

void Platform::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
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
			tilesToMove = std::stoi(newValue);
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
	if (platformType == "Path")
	{
		std::string endBehavior = endPathBehavior;
		if (endBehavior == "")
			endBehavior = "None";

		level << std::to_string(id) << " " << etype << " " << startPosition.x << " " << startPosition.y << " " << spriteIndex << " " << platformType
			<< " " << pathID << " " << pathSpeed << " " << endBehavior << std::endl;
	}
	else
	{
		level << std::to_string(id) << " " << etype << " " << startPosition.x << " " << startPosition.y << " " << spriteIndex << " " << platformType
			<< " " << startVelocity.x << " " << startVelocity.y << " " << tilesToMove << " " << shouldLoop << std::endl;
	}
	
}