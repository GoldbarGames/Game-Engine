#include "Entity.h"
#include "PhysicsInfo.h"
#include <iostream>
#include "Renderer.h"
#include <sstream>
#include "Game.h"
#include "Sprite.h"


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

unsigned int Entity::GetNextValidID()
{
	return nextValidID++;
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
	if (renderer->game->debugMode && drawDebugRect && GetSprite() != nullptr)
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

			if (collider != nullptr && physics != nullptr)
			{
				// draw collider
				targetWidth = collider->bounds->w;
				targetHeight = collider->bounds->h;

				debugSprite->color = { 255, 255, 255, 255 };
				debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));

				Vector2 colliderPosition = Vector2(position.x + collider->offset.x, position.y + collider->offset.y);
				debugSprite->Render(colliderPosition, renderer);
			}
		}
	}
}

void Entity::Render(Renderer * renderer)
{
	if (physics != nullptr)
	{
		//TODO: What is this code for?
		entityPivot = currentSprite->pivot;
	}

	if (currentSprite != nullptr && renderer->IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(position, animator->GetSpeed(), renderer, rotation);
		else
			currentSprite->Render(position, 0, renderer, rotation);
	}
}

void Entity::RenderParallax(Renderer* renderer, float p)
{
	Vector2 renderPosition = Vector2(position.x + (renderer->camera.position.x * p), position.y);

	if (currentSprite != nullptr && renderer->IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(renderPosition, animator->GetSpeed(), renderer, rotation);
		else
			currentSprite->Render(renderPosition, 0, renderer, rotation);
	}
}

void Entity::SetSprite(Sprite* sprite)
{
	currentSprite = sprite;
	currentSprite->scale = scale;
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

void Entity::GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties)
{
	Entity::DeleteProperties(properties);
	std::string id_string = "ID: " + std::to_string(id);
	properties.emplace_back(new Property(new Text(renderer, font, id_string, { 0, 0, 255, 255 })));
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