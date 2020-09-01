#include "Entity.h"
#include "PhysicsComponent.h"
#include <iostream>
#include "Renderer.h"
#include <sstream>
#include "Game.h"
#include "Sprite.h"
#include "Switch.h"

unsigned int Entity::nextValidID = 0;

unsigned int Entity::Size()
{
	unsigned int totalSize = 0;
	totalSize += sizeof(position);

	if (animator != nullptr)
		totalSize += sizeof(*animator);

	if (currentSprite != nullptr)
		totalSize += currentSprite->Size();

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
	totalSize += sizeof(tilesheetIndex);
	totalSize += sizeof(tileCoordinates);
	totalSize += sizeof(impassable);
	totalSize += sizeof(trigger);
	totalSize += sizeof(jumpThru);

	//if (collider != nullptr)
	//	totalSize += sizeof(*collider);

	//if (collisionBounds != nullptr)
	//	totalSize += sizeof(*collisionBounds);

	return totalSize;
}

unsigned int Entity::GetNextValidID()
{
	return nextValidID++;
}

//TODO: Figure out what to do with the background layers
// since they will offset the next valid ID every time we save the level
Entity::Entity(const Vector2& pos)
{
	position = pos;
	id = nextValidID;
	nextValidID++;
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
}

Entity::Entity(const Vector2& pos, Sprite* sprite) : Entity(pos)
{
	currentSprite = sprite;
}

Entity::~Entity()
{
	if (animator != nullptr)
		delete_it(animator);
	if (currentSprite != nullptr)
		delete_it(currentSprite);
	if (collider != nullptr)
		delete_it(collider);
	if (bounds != nullptr)
		delete_it(bounds);
}

void Entity::CreateCollider(float x, float y, float w, float h)
{
	if (collider != nullptr)
		collider->CreateCollider(x, y, w, h);
	else
		collider = new Collider(x, y, w, h);

	CalculateCollider();
}

void Entity::CalculateCollider()
{
	if (collider != nullptr)
		collider->CalculateCollider(position);
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

	if (animator != nullptr)
		animator->Update(this);
	
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
	return currentSprite;
}

void Entity::SetColor(Color newColor)
{
	currentSprite->color = newColor;
}

// NOTE: This returns coordinates where x and y are the center of the rectangle!
// Be careful when using HasIntersection() because you'll get the wrong results
const SDL_Rect* Entity::GetBounds()
{
	if (collider == nullptr)
	{
		if (bounds == nullptr)
		{
			bounds = new SDL_Rect();		
		}
		bounds->x = position.x;
		bounds->y = position.y;
		bounds->w = currentSprite->frameWidth;
		bounds->h = currentSprite->frameHeight;
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
	return Vector2(currentSprite->frameWidth / 2, currentSprite->frameHeight / 2);
}

void Entity::SetPosition(const Vector2& newPosition)
{
	position = newPosition;
}

void Entity::SetAnimator(Animator& anim)
{
	animator = &anim;
	animator->StartTimer();
	animator->DoState(this);
}

void Entity::RenderDebug(const Renderer& renderer)
{
	if (renderer.game->debugMode && drawDebugRect && GetSprite() != nullptr)
	{
		//TODO: Refactor this? It seems like this is not very efficient
		if (debugSprite == nullptr)
			debugSprite = new Sprite(renderer.debugSprite->texture, renderer.debugSprite->shader);

		if (renderer.IsVisible(layer))
		{
			//TODO: Make this a function inside the renderer

			float rWidth = debugSprite->texture->GetWidth();
			float rHeight = debugSprite->texture->GetHeight();

			float targetWidth = GetSprite()->frameWidth;
			float targetHeight = GetSprite()->frameHeight;


			if (impassable || trigger || jumpThru)
			{
				if (impassable)
				{
					debugSprite->color = { 255, 0, 0, 255 };
				}
				else if (trigger)
				{
					debugSprite->color = { 0, 255, 0, 255 };
				}
				else if (jumpThru)
				{
					debugSprite->color = { 255, 255, 0, 255 };
				}

				//debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));
				debugSprite->Render(position, renderer);
			}


			if (collider != nullptr && physics != nullptr)
			{
				// draw collider
				debugSprite->color = { 255, 255, 255, 255 };
				//debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(collider->bounds->w / rWidth, collider->bounds->h / rHeight));
				debugSprite->Render(Vector2(collider->bounds->x, collider->bounds->y), renderer);
			}
		}
	}
}

void Entity::Render(const Renderer& renderer)
{
	if (currentSprite != nullptr && renderer.IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(position, animator->GetSpeed(), renderer, rotation);
		else
			currentSprite->Render(position, 0, renderer, rotation);
	}
}

void Entity::RenderParallax(const Renderer& renderer, float p)
{
	Vector2 renderPosition = Vector2(position.x + (renderer.camera.position.x * p), position.y);

	if (currentSprite != nullptr && renderer.IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(renderPosition, animator->GetSpeed(), renderer, rotation);
		else
			currentSprite->Render(renderPosition, 0, renderer, rotation);
	}
}

void Entity::SetSprite(Sprite& sprite)
{
	currentSprite = &sprite;
	currentSprite->scale = scale;
}

bool Entity::CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera)
{
	if (currentSprite == nullptr)
		return false;

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

void Entity::GetProperties(FontInfo* font, std::vector<Property*>& properties)
{
	Entity::DeleteProperties(properties);
	Property* property = new Property("ID", id);
	property->pType = PropertyType::ReadOnly;
	properties.emplace_back(property);
}

void Entity::DeleteProperties(std::vector<Property*>& properties)
{
	for (unsigned int i = 0; i < properties.size(); i++)
		delete properties[i];

	properties.clear();
}	

void Entity::SetProperty(const std::string& key, const std::string& newValue)
{

}

void Entity::Save(std::ostringstream& level)
{
	// By default, save nothing, because they are probably temp objects like missiles, etc.
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