#include "Platform.h"
#include "../ENGINE/Renderer.h"
#include "../ENGINE/Game.h"
#include "PhysicsComponent.h"
#include "Switch.h"
#include "../ENGINE/globals.h"
#include "../ENGINE/Property.h"
#include "../ENGINE/Editor.h"
#include "MyEditorHelper.h"
#include "../ENGINE/Tile.h"
#include "../ENGINE/Game.h"

Platform::Platform(const Vector2& pos) : MyEntity(pos)
{
	etype = "platform";
	CreateCollider(0, 72, 72, 24);
	layer = DrawingLayer::COLLISION;
	drawOrder = 9;
	
	jumpThru = true;
	impassable = true;

	physics = neww PhysicsComponent(this);
	physics->canBePushed = false; // TODO: Is there some potential here?
	physics->useGravity = false;
	physics->mass = 10;

	shouldSave = true;
}

Platform::~Platform()
{

}

void Platform::Init(const Game& g, const std::string& n)
{
	name = n;

	const std::string tilesheet = "assets/tiles/foresttiles.png";

	tiles.push_back(g.SpawnTile(Vector2(2, 1), tilesheet, position + Vector2(0, TILE_SIZE), DrawingLayer::FRONT));
	tiles.push_back(g.SpawnTile(Vector2(2, 1), tilesheet, position + Vector2(-2 * TILE_SIZE, TILE_SIZE), DrawingLayer::FRONT));
	tiles.push_back(g.SpawnTile(Vector2(2, 1), tilesheet, position + Vector2(2 * TILE_SIZE, TILE_SIZE), DrawingLayer::FRONT));

	for (int i = 0; i < tiles.size(); i++)
	{
		tiles[i]->shouldSave = false;
	}

	if (name == "platform1")
	{
		jumpThru = true;
	}
	else if (name == "platform2")
	{
		jumpThru = false;
	}
}

std::string Platform::CalcDirection(bool x)
{
	Vector2 nextPos = position + physics->velocity;

	if (x)
	{
		// if we need to go to the right...
		if (nextPos.x < currentPath->nodes[pathNodeID]->position.x)
		{
			return "right";
		}
		// if we need to go to the left...
		else if (nextPos.x > currentPath->nodes[pathNodeID]->position.x)
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
		if (nextPos.y < currentPath->nodes[pathNodeID]->position.y)
		{
			return "down";
		}
		// if we need to go up...
		else if (nextPos.y > currentPath->nodes[pathNodeID]->position.y)
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
	physics->previousVelocity = physics->velocity;

	// Draw tiles on front of the platform
	if (name == "platform1" || name == "platform2")
	{
		if (tiles.size() == 3)
		{
			tiles[0]->position = position + Vector2(0, TILE_SIZE);
			tiles[1]->position = position + Vector2(-2 * TILE_SIZE, TILE_SIZE);
			tiles[2]->position = position + Vector2(2 * TILE_SIZE, TILE_SIZE);
		}
	}

	if (attachedSwitch != nullptr)
	{
		lastPosition = position;

		if (attachedSwitch->GetAnimator()->GetBool("isPressed"))
		{
			LerpVector2(position, startPosition + switchDistanceMoved, 30.0f, 2.0f);
		}
		else
		{
			LerpVector2(position, startPosition, 30.0f, 2.0f);
		}

		// Set velocity based on the distance traveled
		physics->velocity = Vector2((position.x - lastPosition.x) / game.dt, 
			(position.y - lastPosition.y) / game.dt);

		if (collider != nullptr)
		{
			CalculateCollider();
		}
	}
	else if (switchID > -1)
	{
		bool foundSwitch = false;
		for (int i = 0; i < game.entities.size(); i++)
		{
			if (game.entities[i]->id == switchID && game.entities[i]->etype == "switch")
			{
				//attachedSwitch = static_cast<Switch*>(game.entities[i]);
				attachedSwitch = game.entities[i];
				foundSwitch = true;
				break;
			}
		}

		if (!foundSwitch)
		{
			switchID = -1;
		}
	}
	else
	{
		// TODO: Put moving platform code here
		physics->velocity = Vector2(0, 0);
	}

	if (platformType == "Path")
	{
		if (currentPath == nullptr && pathID > -1)
		{
			// Find the path and assign it
			bool foundPath = false;
			for (int i = 0; i < game.entities.size(); i++)
			{
				if (game.entities[i]->id == pathID && game.entities[i]->etype == "path")
				{
					currentPath = static_cast<Path*>(game.entities[i]);
					foundPath = true;
					break;
				}
			}

			if (!foundPath)
			{
				pathID = -1;
			}
		}
		else if (currentPath != nullptr) // if we have the path, move to the next point
		{
			if (pathNodeID >= currentPath->nodes.size())
				return;

			// If we just changed directions, wait a bit before moving again
			// TODO: Customize the delay time for each platform/path
			if (wasMovingForward != movingForwardOnPath)
			{
				delayCounter++;
				if (delayCounter < delayMax)
					return;
			}

			delayCounter = 0;
			wasMovingForward = movingForwardOnPath;
			lastPosition = position;

			LerpVector2(position, currentPath->nodes[pathNodeID]->position, pathSpeed, 2.0f);

			if (position.RoundToInt() == currentPath->nodes[pathNodeID]->position.RoundToInt())
			{
				// loop for now
				if (movingForwardOnPath)
				{
					pathNodeID++;					
					if (pathNodeID >= currentPath->nodes.size())
					{
						movingForwardOnPath = false;
						pathNodeID--;
					}						
				}
				else
				{
					pathNodeID--;
					if (pathNodeID < 0)
					{
						movingForwardOnPath = true;
						pathNodeID++;
					}						
				}

			}

			// Set velocity based on the distance traveled
			physics->velocity = Vector2((position.x - lastPosition.x) / game.dt,
				(position.y - lastPosition.y) / game.dt);

			if (collider != nullptr)
			{
				CalculateCollider();
			}
		}
	}

	// NOTE: We do not want to update this the normal way here,
	// because it is already changing its position,
	// so we do not want to do it again

	return;
}

void Platform::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Platform::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Collider Pos X", (int)collider->offset.x));
	properties.emplace_back(new Property("Collider Pos Y", (int)collider->offset.y));
	properties.emplace_back(new Property("Collider Width", collider->scale.x));
	properties.emplace_back(new Property("Collider Height", collider->scale.y));

	properties.emplace_back(new Property("Switch ID", switchID));

	if (switchID > -1)
	{
		properties.emplace_back(new Property("Switch Distance X", switchDistanceMoved.x));
		properties.emplace_back(new Property("Switch Distance Y", switchDistanceMoved.y));
	}

	properties.emplace_back(new Property("Platform Type", platformType));
	if (platformType == "Path")
	{
		properties.emplace_back(new Property("Path ID", pathID));
		properties.emplace_back(new Property("Speed", pathSpeed));
		properties.emplace_back(new Property("Delay", delayMax));
	}

	/*	
	const std::vector<std::string> platformTypes = { "Idle", "Move", "Path" };
	properties.emplace_back(new Property("Platform Type", platformType, platformTypes));
	*/
}

void Platform::SetProperty(const std::string& key, const std::string& newValue)
{
	try
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
			shouldLoop = (newValue == "True") ? true : false;
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
		else if (key == "Switch ID")
		{
			if (newValue != "")
				switchID = std::stoi(newValue);

			if (switchID == -1)
			{
				attachedSwitch = nullptr;
			}
		}
		else if (key == "Switch Distance X")
		{
			if (newValue != "")
				switchDistanceMoved.x = std::stoi(newValue);
		}
		else if (key == "Switch Distance Y")
		{
			if (newValue != "")
				switchDistanceMoved.y = std::stoi(newValue);
		}
	}
	catch (std::exception ex)
	{
		std::cout << ex.what() << std::endl;
	}
	
}

void Platform::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);

	if (platformType == "Path")
	{
		map["subtype"] = std::to_string(subtype);
		map["platformType"] = platformType;
		map["pathID"] = std::to_string(pathID);
		map["pathSpeed"] = std::to_string((int)pathSpeed);
		map["endBehavior"] = endPathBehavior == "" ? "None" : endPathBehavior;
	}
	else
	{
		map["subtype"] = std::to_string(subtype);
		map["platformType"] = platformType;
		map["vx"] = std::to_string((int)startVelocity.x);
		map["vy"] = std::to_string((int)startVelocity.y);
		map["tilesToMove"] = std::to_string(tilesToMove);
		map["shouldLoop"] = std::to_string(shouldLoop);
	}

	map["switchID"] = std::to_string(switchID);
	map["switchDX"] = std::to_string((int)switchDistanceMoved.x);
	map["switchDY"] = std::to_string((int)switchDistanceMoved.y);
}

void Platform::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	subtype = std::stoi(map["subtype"]);
	platformType = map["platformType"];

	if (platformType == "Path")
	{
		pathID = std::stoi(map["pathID"]);
		pathSpeed = std::stof(map["pathSpeed"]);
		endPathBehavior = map["endPathBehavior"];

		if (game.editor->helper != nullptr)
		{
			MyEditorHelper* helper = static_cast<MyEditorHelper*>(game.editor->helper);
			helper->loadListMovingPlatforms.push_back(this);
		}
	}
	else
	{
		float vx = std::stof(map["vx"]);
		float vy = std::stof(map["vy"]);
		startVelocity = Vector2(vx, vy);
		tilesToMove = std::stoi(map["tilesToMove"]);
		shouldLoop = std::stoi(map["shouldLoop"]);
		physics->SetVelocity(startVelocity);
	}

	switchID = std::stoi(map["switchID"]);

	if (switchID > -1)
	{
		switchDistanceMoved.x = std::stoi(map["switchDX"]);
		switchDistanceMoved.y = std::stoi(map["switchDY"]);
	}
}