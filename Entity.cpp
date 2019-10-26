#include "Entity.h"
#include "debug_state.h"
#include <iostream>
#include "Renderer.h"
#include <sstream>

unsigned int Entity::nextValidID = 0;

Entity::Entity(Vector2 pos)
{
	position = pos;
	id = nextValidID;
	nextValidID++;
}

Entity::~Entity()
{
	if (animator != nullptr)
		delete animator;
	if (currentSprite != nullptr)
		delete currentSprite;
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
	return Vector2(currentSprite->windowRect.w/2, currentSprite->windowRect.h / 2);
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
		if (impassable)
			SDL_SetRenderDrawColor(renderer->renderer, 255, 0, 0, 255);
		else
			SDL_SetRenderDrawColor(renderer->renderer, 0, 255, 0, 255);

		SDL_RenderDrawRect(renderer->renderer, currentSprite->GetRect());
		SDL_SetRenderDrawColor(renderer->renderer, 0, 0, 0, 255);
	}
}

void Entity::Render(Renderer * renderer, Vector2 cameraOffset)
{
	if (currentSprite != nullptr && renderer->IsVisible(layer))
	{
		if (animator != nullptr)
			currentSprite->Render(position - cameraOffset, animator->speed, animator->animationTimer.GetTicks(), flip, renderer);
		else
			currentSprite->Render(position - cameraOffset, 0, -1, flip, renderer);

		RenderDebug(renderer, cameraOffset);
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

void Entity::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Text*>& properties)
{
	Entity::DeleteProperties(properties);
	std::string id_string = "ID: " + std::to_string(id);
	properties.emplace_back(new Text(renderer, font, id_string, { 255, 0, 0, 255 }));
}

void Entity::DeleteProperties(std::vector<Text*>& properties)
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