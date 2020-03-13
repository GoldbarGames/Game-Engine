#include "Entity.h"
#include "PhysicsEntity.h"
#include "debug_state.h"
#include <iostream>
#include "Renderer.h"
#include <sstream>
#include "Game.h"

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
	//totalSize += sizeof(isPhysicsEntity);
	totalSize += sizeof(drawDebugRect);
	totalSize += sizeof(name);
	totalSize += sizeof(flip);
	totalSize += sizeof(entityPivot);
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


//TODO: Figure out what to do with the background layers
// since they will offset the next valid ID every time we save the level
Entity::Entity(Vector2 pos)
{
	position = Vector2(pos.x, pos.y);
	//position = pos;
	id = nextValidID;
	nextValidID++;
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
}

Entity::Entity(Vector2 pos, Sprite* sprite)
{
	position = pos;
	id = nextValidID;
	nextValidID++;
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
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
}

void Entity::CreateCollider(float x, float y, float w, float h)
{
	if (collisionBounds != nullptr)
		delete collisionBounds;

	collisionBounds = new SDL_Rect();
	collisionBounds->x = (int)x;
	collisionBounds->y = (int)y;
	collisionBounds->w = w;
	collisionBounds->h = h;

	colliderScale.x = w;
	colliderScale.y = h;
}

void Entity::CalculateCollider()
{
	if (etype == "door")
	{
		int test = 0;
	}

	if (currentSprite != nullptr)
	{
		collisionBounds->w = colliderScale.x; //currentSprite->frameWidth;
		collisionBounds->h = colliderScale.y; //currentSprite->frameHeight;
	}
	else
	{
		collisionBounds->w = 1;
		collisionBounds->h = 1;
	}

	// TODO: Do we need this anymore? Not sure this is the right idea
	//collisionBounds->x = (int)(position.x - (collisionBounds->w / 2) + colliderOffset.x);
	//collisionBounds->y = (int)(position.y - (collisionBounds->h / 2) + colliderOffset.y);

	// set the collision bounds to the position
	collisionBounds->x = (int)(position.x);
	collisionBounds->y = (int)(position.y);
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

const SDL_Rect* Entity::GetBounds()
{
	//if (collisionBounds == nullptr)
	//	return currentSprite->GetRect();
	//else
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

void Entity::RenderDebug(Renderer * renderer)
{
	if (GetModeDebug() && drawDebugRect && GetSprite() != nullptr)
	{
		if (debugSprite == nullptr)
			debugSprite = new Sprite(renderer->debugSprite->texture, renderer->debugSprite->shader);

		if (renderer->IsVisible(layer))
		{
			//TODO: Make this a function inside the renderer
			float rWidth = debugSprite->texture->GetWidth();
			float rHeight = debugSprite->texture->GetHeight();

			float targetWidth = GetSprite()->frameWidth;
			float targetHeight = GetSprite()->frameHeight;

			if (jumpThru)
				debugSprite->color = { 255, 255, 0, 255 };
			else if (impassable)
				debugSprite->color = { 255, 0, 0, 255 };
			else
				debugSprite->color = { 0, 255, 0, 255 };

			debugSprite->pivot = GetSprite()->pivot;
			debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));
			debugSprite->Render(position, renderer);

			if (physics != nullptr)
			{
				// draw collider
				targetWidth = collisionBounds->w;
				targetHeight = collisionBounds->h;

				debugSprite->color = { 255, 255, 255, 255 };
				debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));

				Vector2 colliderPosition = Vector2(position.x + colliderOffset.x,
					position.y + colliderOffset.y);
				debugSprite->Render(colliderPosition, renderer);
			}
		}
	}
}

void Entity::Render(Renderer * renderer)
{
	if (physics != nullptr)
	{
		//TODO: What is all of this code for? Why do we need this offset?
		// Is it so that when you turn around, the collision box always stays centered?
		entityPivot = currentSprite->pivot;

		// Get center of the white collision box, and use it as a vector2
		float collisionCenterX = (collisionBounds->x + (collisionBounds->w / 2.0f));
		float collisionCenterY = (collisionBounds->y + (collisionBounds->h / 2.0f));
		Vector2 collisionCenter = Vector2(collisionCenterX + colliderOffset.x, collisionCenterY + colliderOffset.y);

		Vector2 scaledPivot = physics->CalcScaledPivot();
		Vector2 offset = collisionCenter - scaledPivot;

		if (GetModeEdit())
		{
			if (animator != nullptr)
				currentSprite->Render(position, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, rotation);
			else
				currentSprite->Render(position, 0, -1, flip, renderer, rotation);
		}
		else // use offset here?
		{
			if (animator != nullptr)
				currentSprite->Render(position, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, rotation);
			else
				currentSprite->Render(position, 0, -1, flip, renderer, rotation);
		}

	}
	else
	{
		if (layer == DrawingLayer::COLLISION2)
			int test = 0;

		if (currentSprite != nullptr && renderer->IsVisible(layer))
		{
			if (animator != nullptr)
				currentSprite->Render(position, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, rotation);
			else
				currentSprite->Render(position, 0, -1, flip, renderer, rotation);
		}
	}
}

void Entity::RenderParallax(Renderer* renderer, float p)
{
	Vector2 renderPosition = Vector2(position.x + (renderer->camera.position.x * p), position.y);

	if (currentSprite != nullptr && renderer->IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(renderPosition, animator->GetSpeed(), animator->animationTimer.GetTicks(), flip, renderer, rotation);
		else
			currentSprite->Render(renderPosition, 0, -1, flip, renderer, rotation);
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