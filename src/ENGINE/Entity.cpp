#include "Entity.h"

#include <iostream>
#include "Renderer.h"
#include <sstream>
#include "Game.h"
#include "Sprite.h"
#include "Property.h"
#include "Editor.h"
#include "Animator.h"
#include "AnimatorInfo.h"
#include "globals.h"

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
	//totalSize += sizeof(shouldDelete);
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
				std::cout << "ERROR generating new ID - overflow" << std::endl;
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
			std::cout << "ERROR generating new ID - overflow" << std::endl;
			break;
		}			

	} while (takenIDs.find(i) != takenIDs.end());

	nextValidID = i;
	return nextValidID;*/
}

//TODO: Figure out what to do with the background layers
// since they will offset the next valid ID every time we save the level
Entity::Entity(const glm::vec3& pos) : collider(0, 0, Globals::TILE_SIZE, Globals::TILE_SIZE)
{
	name = "entity";
	position = pos;
	startPosition = position;

	id = GenerateValidID();
}

Entity::Entity(const glm::vec3& pos, Sprite* sprite) : Entity(pos)
{
	currentSprite = *sprite;
}

Entity::~Entity()
{
	if (animator != nullptr)
	{
		delete_it(animator);
	}
}

void Entity::Init(const Game& g, const std::string& n)
{
	name = n;
}

void Entity::CreateCollider(float x, float y, float w, float h)
{
	collider.offset.x = x;
	collider.offset.y = y;
	collider.scale.x = w;
	collider.scale.y = h;

	CalculateCollider();
}

void Entity::CalculateCollider()
{
	collider.CalculateCollider(position, rotation);
}


void Entity::Pause(uint32_t ticks)
{

}


void Entity::Unpause(uint32_t ticks)
{

}

void Entity::Update(Game& game)
{	
	game.updateCalls++;
	lastPosition = position;

	currentSprite.color = color;

	if (animator != nullptr)
		animator->Update(*this);

	//if (shader != nullptr && currentSprite.shader != shader)
	//	currentSprite.SetShader(shader);
	
	CalculateCollider();
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
	currentSprite.color = newColor;
}

// NOTE: This returns coordinates where x and y are the center of the rectangle!
// Be careful when using HasIntersection() because you'll get the wrong results
const SDL_Rect* Entity::GetBounds()
{
	return &collider.bounds;
}

SDL_Rect Entity::GetTopLeftBounds() const
{
	return ConvertCoordsFromCenterToTopLeft(collider.bounds);
}

glm::vec3 Entity::GetPosition() const
{
	return position;
}

glm::vec3 Entity::GetCenter() const
{
	return glm::vec3(currentSprite.frameWidth / 2, currentSprite.frameHeight / 2, 0);
}

void Entity::SetPosition(const glm::vec3& newPosition)
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

			if (etype != "tile")
			{
				renderer.RenderDebugRect(collider.bounds, scale);
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
	glm::vec3 renderPosition = glm::vec3(position.x + (renderer.camera.position.x * p), position.y, position.z);

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
bool Entity::CanSpawnHere(const glm::vec3& spawnPosition, const Game& game, bool useCamera)
{
	bool shouldSpawn = true;

	if (etype == "door")
	{
		//TODO: Maybe there's a better way to initialize the bounds for the sprite
		SDL_Rect myBounds = *(GetBounds());
		myBounds.x = (int)spawnPosition.x;
		myBounds.y = (int)spawnPosition.y;

		return true;

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
	properties.emplace_back(new Property("ID", id));
	properties.emplace_back(new Property("Subtype", subtype));
	properties.emplace_back(new Property("Layer", (int)layer));
}

void Entity::DeleteProperties(std::vector<Property*>& properties)
{
	for (unsigned int i = 0; i < properties.size(); i++)
		delete_it(properties[i]);

	properties.clear();
}	

void Entity::SetProperty(const std::string& key, const std::string& newValue, std::vector<Property*>& properties)
{
	for (int i = 0; i < properties.size(); i++)
	{
		if (properties[i]->key == key)
		{
			properties[i]->SetProperty(newValue);
		}
	}
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
		
		if (currentSprite.shader == nullptr)
			map["shader"] = "1";
		else
			map["shader"] = std::to_string(currentSprite.shader->GetName());

		map["name"] = name;
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

	static const std::string rotationZ = "rotationZ";
	static const std::string shaderString = "shader";
	static const std::string subtypeString = "subtype";
	static const std::string nameString = "name";

	if (map.count(rotationZ) != 0)
	{
		if (map[rotationZ] == "")
			rotation.z = 0;
		else
			rotation.z = std::stoi(map[rotationZ]);
	}

	if (map.count(shaderString) != 0)
	{
		//if (map[shaderString] != "" && game.renderer.shaders.count(std::stoi(map[shaderString])) != 0)
		//	shader = game.renderer.shaders[std::stoi(map[shaderString])];
	}
}


void Entity::OnClick(Uint32 mouseState, Game& game)
{
	//std::cout << "Clicked, pressed on " << etype << "!" << std::endl;
}

void Entity::OnClickPressed(Uint32 mouseState, Game& game) 
{
	//std::cout << "Clicked, pressed down on " << etype << "!" << std::endl;
	if (draggable && game.draggedEntity == nullptr)
	{
		game.draggedEntity = this;
	}
}

void Entity::OnClickReleased(Uint32 mouseState, Game& game)
{
	//std::cout << "Clicked, released on " << etype << "!" << std::endl;
	if (draggable && game.draggedEntity == this)
	{
		game.draggedEntity = nullptr;
	}
}

glm::vec2 Entity::GetScale() const
{ 
	return scale; 
}

void Entity::SetScale(const glm::vec2& newScale)
{
	scale = newScale;
}

int Entity::GetGridSize()
{
	return Globals::TILE_SIZE;
}