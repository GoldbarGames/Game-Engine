#include "Entity.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"

#include <iostream>
#include "Renderer.h"
#include <sstream>
#include "Game.h"
#include "Sprite.h"
#include "Property.h"
#include "Editor.h"
#include "Animator.h"
#include "AnimatorInfo.h"

uint32_t Entity::nextValidID = 0;
std::unordered_map<unsigned int, bool> Entity::takenIDs;

unsigned int Entity::Size()
{
	unsigned int totalSize = 0;
	totalSize += sizeof(position);

	if (animator != nullptr)
		totalSize += sizeof(*animator);

	//totalSize += sizeof(colliderWidth);
	//totalSize += sizeof(colliderHeight);
	totalSize += sizeof(nextValidID);
	totalSize += sizeof(drawDebugRect);
	totalSize += sizeof(name);
	//totalSize += sizeof(flip);
	//totalSize += sizeof(entityPivot);
	totalSize += sizeof(shouldDelete);
	totalSize += sizeof(etype);
	totalSize += sizeof(id);
	totalSize += sizeof(drawOrder);
	totalSize += sizeof(layer);
	//totalSize += sizeof(tilesheetIndex);
	//totalSize += sizeof(tileCoordinates);
	totalSize += sizeof(impassable);
	totalSize += sizeof(trigger);
	totalSize += sizeof(jumpThru);

	//if (collider != nullptr)
	//	totalSize += sizeof(*collider);

	//if (collisionBounds != nullptr)
	//	totalSize += sizeof(*collisionBounds);

	return totalSize;
}

// TODO: We don't just want to always increase this,
// we want to make sure that there are no gaps
uint32_t Entity::GenerateValidID()
{
	// If the next ID has already been taken, increment by 1
	if (takenIDs.count(nextValidID) != 0)
	{
		unsigned int i = nextValidID;
		do
		{
			i++;

			if (i == 0)
			{
				std::cout << "ERROR generating neww ID - overflow" << std::endl;
				break;
			}

		} while (takenIDs.count(i) != 0);
		nextValidID = i;
	}

	takenIDs[nextValidID] = true;
	return nextValidID;

	/*
	
	if (takenIDs.find(nextValidID) == takenIDs.end())
	{
		return nextValidID;
	}

	return nextValidID;*/

	// If the nextValidID is already taken,
	// loop through all taken IDs and fill in the gaps

	// TODO: This takes way too long and doesn't work right anyway,
	// so find a better way to do this!

	/*
	unsigned int i = 1;
	do
	{
		i++;

		if (i == 0)
		{
			std::cout << "ERROR generating neww ID - overflow" << std::endl;
			break;
		}			

	} while (takenIDs.find(i) != takenIDs.end());

	nextValidID = i;
	return nextValidID;*/
}

//TODO: Figure out what to do with the background layers
// since they will offset the next valid ID every time we save the level
Entity::Entity(const Vector2& pos)
{
	name = "entity";
	position = pos;
	startPosition = position;

	id = GenerateValidID();
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
}

Entity::Entity(const Vector2& pos, Sprite* sprite) : Entity(pos)
{
	currentSprite = *sprite;
}

Entity::~Entity()
{
	if (animator != nullptr)
	{
		delete_it(animator);
	}
		
	if (collider != nullptr)
		delete_it(collider);
	if (bounds != nullptr)
		delete_it(bounds);
	if (physics != nullptr)
		delete_it(physics);
	if (health != nullptr)
		delete_it(health);
}

void Entity::Init(const Game& g, const std::string& n)
{
	name = n;
}

void Entity::CreateCollider(float x, float y, float w, float h)
{
	if (collider != nullptr)
		collider->CreateCollider(x, y, w, h);
	else
		collider = neww Collider(x, y, w, h);

	CalculateCollider();
}

void Entity::CalculateCollider()
{
	if (collider != nullptr)
		collider->CalculateCollider(position, rotation);
}


void Entity::Pause(Uint32 ticks)
{
	if (animator != nullptr)
	{
		//std::cout << "-- pausing --" << std::endl;
		//animator->animationTimer.Pause();
	}	
}


void Entity::Unpause(Uint32 ticks)
{
	if (animator != nullptr)
	{
		//std::cout << "-- unpausing --" << std::endl;
		//animator->animationTimer.Unpause();
	}
}

void Entity::Update(Game& game)
{	
	game.updateCalls++;
	lastPosition = position;

	currentSprite.color = color;

	if (animator != nullptr)
		animator->Update(*this);
	
	CalculateCollider();
	if (physics != nullptr)
		physics->Update(game);
}

Animator* Entity::GetAnimator()
{
	return animator;
}

Sprite* Entity::GetSprite()
{
	return &currentSprite;
}

void Entity::SetColor(Color newColor)
{
	color = newColor;
}

// NOTE: This returns coordinates where x and y are the center of the rectangle!
// Be careful when using HasIntersection() because you'll get the wrong results
const SDL_Rect* Entity::GetBounds()
{
	if (collider == nullptr)
	{
		if (bounds == nullptr)
		{
			bounds = neww SDL_Rect();		
		}
		bounds->x = position.x;
		bounds->y = position.y;
		bounds->w = currentSprite.frameWidth;
		bounds->h = currentSprite.frameHeight;
		return bounds;
	}
	else
	{
		return collider->bounds;
	}		
}

Vector2 Entity::GetPosition() const
{
	return position;
}

Vector2 Entity::GetCenter() const
{
	return Vector2(currentSprite.frameWidth / 2, currentSprite.frameHeight / 2);
}

void Entity::SetPosition(const Vector2& newPosition)
{
	position = newPosition;
}

void Entity::SetAnimator(Animator& anim)
{
	animator = &anim;
	animator->DoState(*this);
}

void Entity::RenderDebug(const Renderer& renderer)
{
	if (renderer.game->debugMode && drawDebugRect && GetSprite() != nullptr)
	{
		if (renderer.IsVisible(layer))
		{
			if (impassable || trigger || jumpThru)
			{
				static SDL_Rect rect;
				rect.x = position.x;
				rect.y = position.y;
				rect.w = currentSprite.frameWidth;
				rect.h = currentSprite.frameHeight;
				static Color debugColor = { 255, 255, 255, 255 };

				if (impassable)
				{
					debugColor = { 255, 0, 0, 255 };
				}
				else if (trigger)
				{
					debugColor = { 0, 255, 0, 255 };
				}
				else if (jumpThru)
				{
					debugColor = { 255, 255, 0, 255 };
				}

				renderer.RenderDebugRect(rect, scale, debugColor);
			}

			if (collider != nullptr && physics != nullptr)
			{
				renderer.RenderDebugRect(*collider->bounds, scale);
			}
		}
	}
}

void Entity::Render(const Renderer& renderer)
{
	if (renderer.IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite.Render(position, animator->GetSpeed(), renderer, scale, rotation);
		else
			currentSprite.Render(position, 0, renderer, scale, rotation);
	}
}

void Entity::RenderParallax(const Renderer& renderer, float p)
{
	Vector2 renderPosition = Vector2(position.x + (renderer.camera.position.x * p), position.y);

	if (renderer.IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite.Render(renderPosition, animator->GetSpeed(), renderer, scale, rotation);
		else
			currentSprite.Render(renderPosition, 0, renderer, scale, rotation);
	}
}

void Entity::SetSprite(Sprite& sprite)
{
	currentSprite = sprite;
}

// This should check to see whether or not there already exists an object
// where we are trying to place this object, and possibly (depending on this object)
// whether or not there is any ground below (classes can override this if needed)
bool Entity::CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera)
{
	bool shouldSpawn = true;

	if (etype == "door")
	{
		//TODO: Maybe there's a better way to initialize the bounds for the sprite
		SDL_Rect myBounds = *(GetBounds());
		myBounds.x = (int)spawnPosition.x;
		myBounds.y = (int)spawnPosition.y;

		SDL_Rect tileBelowMyBoundsLeft = myBounds;
		tileBelowMyBoundsLeft.y += myBounds.h;
		tileBelowMyBoundsLeft.w = game.editor->GRID_SIZE;
		tileBelowMyBoundsLeft.h = game.editor->GRID_SIZE;

		SDL_Rect tileBelowMyBoundsRight = myBounds;
		tileBelowMyBoundsRight.x += game.editor->GRID_SIZE;
		tileBelowMyBoundsRight.y += myBounds.h;
		tileBelowMyBoundsRight.w = game.editor->GRID_SIZE;
		tileBelowMyBoundsRight.h = game.editor->GRID_SIZE;

		bool hasGroundLeft = false;
		bool hasGroundRight = false;

		for (unsigned int i = 0; i < game.entities.size(); i++)
		{
			const SDL_Rect* theirBounds = game.entities[i]->GetBounds();

			// 1. Check to make sure that this door does NOT intersect with any other doors
			// Check to make sure that we can't place inside a solid tile
			if (game.entities[i]->etype == etype || game.entities[i]->impassable)
			{
				if (HasIntersection(myBounds, *theirBounds))
				{
					shouldSpawn = false;
				}
			}

			// 2. Check to make sure that this door is one tile above a tile on the foreground layer
			if (game.entities[i]->impassable)
			{
				if (HasIntersection(tileBelowMyBoundsLeft, *theirBounds))
				{
					hasGroundLeft = true;
				}

				if (HasIntersection(tileBelowMyBoundsRight, *theirBounds))
				{
					hasGroundRight = true;
				}
			}
		}

		shouldSpawn = !hasGroundLeft || !hasGroundRight;
	}
	else
	{
		for (unsigned int i = 0; i < game.entities.size(); i++)
		{
			if (game.entities[i]->GetPosition() == spawnPosition &&
				game.entities[i]->layer == layer &&
				game.entities[i]->etype == etype)
			{
				return false;
			}
		}
	}

	return shouldSpawn;
}

void Entity::OnTriggerStay(Entity& other, Game& game)
{

}

void Entity::OnTriggerEnter(Entity& other, Game& game)
{

}

void Entity::OnTriggerExit(Entity& other, Game& game)
{

}

void Entity::GetProperties(std::vector<Property*>& properties)
{
	Entity::DeleteProperties(properties);
	// TODO: MEMORY LEAK
	Property* property = neww Property("ID", id);
	property->pType = PropertyType::ReadOnly;
	properties.emplace_back(property);
}

void Entity::DeleteProperties(std::vector<Property*>& properties)
{
	for (unsigned int i = 0; i < properties.size(); i++)
		delete_it(properties[i]);

	properties.clear();
}	

void Entity::SetProperty(const std::string& key, const std::string& newValue)
{

}


void Entity::Save(std::unordered_map<std::string, std::string>& map)
{
	// By default, save nothing, because they are probably temp objects like missiles, etc.
	if (shouldSave)
	{
		// Save each variable to a map, don't output anything
		map["id"] = std::to_string(id);
		map["type"] = etype;
		map["positionX"] = std::to_string((int)startPosition.x);
		map["positionY"] = std::to_string((int)startPosition.y);
		map["rotationZ"] = std::to_string((int)rotation.z);
		map["subtype"] = std::to_string(subtype);
	}
}

void Entity::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	// We only want to call this function once, when the instance is spawned.
	// Since we know that id starts at 0, we should only generate an ID
	// when the id is 0. Otherwise, this entity already has a valid ID.
	if (id == 0)
	{
		nextValidID = std::stoi(map["id"]);
		id = GenerateValidID();
	}		

	rotation.z = std::stoi(map["rotationZ"]);
}


void Entity::OnClick(Uint32 mouseState, Game& game)
{

}

void Entity::OnClickPressed(Uint32 mouseState, Game& game) 
{
	std::cout << "Clicked, pressed down on " << etype << "!" << std::endl;
}

void Entity::OnClickReleased(Uint32 mouseState, Game& game)
{

}

Vector2 Entity::GetScale() const 
{ 
	return scale; 
}

void Entity::SetScale(const Vector2& newScale) 
{
	scale = newScale;
}