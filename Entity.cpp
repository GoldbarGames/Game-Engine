#include "Entity.h"
#include "PhysicsEntity.h"
#include "debug_state.h"
#include <iostream>
#include "Renderer.h"
#include <sstream>
#include "Game.h"

unsigned int Entity::nextValidID = 0;

//TODO: Figure out what to do with the background layers
// since they will offset the next valid ID every time we save the level
Entity::Entity(Vector2 pos)
{
	position = pos;
	id = nextValidID;
	nextValidID++;
	CreateCollider(TILE_SIZE, TILE_SIZE, 0, 0, TILE_SIZE, TILE_SIZE);
}

Entity::Entity(Vector2 pos, Sprite* sprite)
{
	position = pos;
	id = nextValidID;
	nextValidID++;
	CreateCollider(TILE_SIZE, TILE_SIZE, 0, 0, TILE_SIZE, TILE_SIZE);
	currentSprite = sprite;
}

Entity::~Entity()
{
	if (animator != nullptr)
		delete animator;
	if (currentSprite != nullptr)
		delete currentSprite;
	if (collisionBounds != nullptr)
		delete collisionBounds;
	if (collider != nullptr)
		delete collider;
}

void Entity::CreateCollider(float startX, float startY, float x, float y, float w, float h)
{
	if (collider != nullptr)
		delete collider;

	collider = new SDL_Rect();
	collider->x = (int)x;
	collider->y = (int)y;
	collider->w = (int)1;
	collider->h = (int)1;

	colliderWidth = w;
	colliderHeight = h;

	if (collisionBounds != nullptr)
		delete collisionBounds;

	collisionBounds = new SDL_Rect();
	collisionBounds->x = (int)x;
	collisionBounds->y = (int)y;
	collisionBounds->w = 1;
	collisionBounds->h = 1;

	//startSpriteSize.x = startX * Renderer::GetScale();
	//startSpriteSize.y = startY * Renderer::GetScale();
}

void Entity::CalculateCollider(Vector2 cameraOffset)
{
	// set the collision bounds position to where the player actually is
	collisionBounds->x = (int)(position.x + collider->x);
	collisionBounds->y = (int)(position.y + collider->y);

	// scale the bounds of the sprite by a number to set the collider's width and height
	collisionBounds->w = (int)colliderWidth;
	collisionBounds->h = (int)colliderHeight;
}


void Entity::Pause(Uint32 ticks)
{
	if (animator != nullptr)
	{
		//std::cout << "-- pausing --" << std::endl;
		animator->animationTimer.Pause(ticks);
	}	
}

void Entity::Unpause(Uint32 ticks)
{
	if (animator != nullptr)
	{
		//std::cout << "-- unpausing --" << std::endl;
		animator->animationTimer.Unpause(ticks);
	}
}

void Entity::Update(Game& game)
{
	CalculateCollider(Vector2(0,0));
	if (animator != nullptr)
		animator->Update(this);
}

Animator* Entity::GetAnimator()
{
	return animator;
}

Sprite* Entity::GetSprite()
{
	return currentSprite;
}

const SDL_Rect* Entity::GetBounds()
{
	if (collisionBounds == nullptr)
		return currentSprite->GetRect();
	else
		return collisionBounds;
}

Vector2 Entity::GetPosition()
{
	return position;
}

Vector2 Entity::GetCenter()
{
	return Vector2(currentSprite->frameWidth / 2, currentSprite->frameHeight / 2);
}

void Entity::SetPosition(Vector2 newPosition)
{
	position = newPosition;
}

void Entity::SetAnimator(Animator * anim)
{
	animator = anim;
	anim->StartTimer();
	anim->DoState(this);
}

void Entity::RenderDebug(Renderer * renderer, Vector2 cameraOffset)
{
	if (GetModeDebug() && currentSprite != nullptr)
	{
		if (jumpThru)
			SDL_SetRenderDrawColor(renderer->renderer, 255, 165, 0, 255);
		else if (impassable)
			SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 255);
		else
			SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 0, 255);

		SDL_RenderDrawRect(renderer->renderer, currentSprite->GetRect());
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
}

void Entity::Render(Renderer * renderer)
{
	if (currentSprite != nullptr && renderer->IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(position, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, 0);
		else
			currentSprite->Render(position, 0, -1, flip, renderer, 0);

		RenderDebug(renderer, Vector2(0,0));
	}
}

void Entity::SetSprite(Sprite* sprite)
{
	currentSprite = sprite;
}

bool Entity::CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera)
{
	return true;
}

float Entity::CalcCollisionVelocity(PhysicsEntity* other, bool x)
{
	return 0;
}

bool Entity::IsEntityPushingOther(PhysicsEntity* other, bool x)
{
	return false;
}

void Entity::OnTriggerStay(Entity* other, Game& game)
{

}

void Entity::OnTriggerEnter(Entity* other, Game& game)
{

}

void Entity::OnTriggerExit(Entity* other, Game& game)
{

}

void Entity::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::DeleteProperties(properties);
	std::string id_string = "ID: " + std::to_string(id);
	properties.emplace_back(new Property(new Text(renderer, font, id_string, { 255, 0, 0, 255 })));
}

void Entity::DeleteProperties(std::vector<Property*>& properties)
{
	for (unsigned int i = 0; i < properties.size(); i++)
		delete properties[i];

	properties.clear();
}	

void Entity::SetProperty(std::string prop, std::string newValue)
{

}

void Entity::Save(std::ostringstream& level)
{
	// By default, save nothing, because they are probably temp objects like missiles, etc.
}