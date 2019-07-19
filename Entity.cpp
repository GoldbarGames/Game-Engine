#include "Entity.h"
#include "debug_state.h"

Entity::Entity()
{

}

Entity::~Entity()
{
	
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

const SDL_Rect* Entity::GetBounds()
{
	return currentSprite->GetRect();
}

Vector2 Entity::GetPosition()
{
	return position;
}

void Entity::SetPosition(Vector2 newPosition)
{
	position = newPosition;
}

void Entity::SetAnimator(Animator * anim)
{
	animator = anim;
	anim->DoState(this);
}

void Entity::Render(SDL_Renderer * renderer, Vector2 cameraOffset)
{
	if (currentSprite != nullptr)
	{
		if (animator != nullptr)
			currentSprite->Render(position - cameraOffset, animator->speed, renderer);
		else
			currentSprite->Render(position - cameraOffset, 0, renderer);

		if (GetModeDebug())
		{
			if (impassable)
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			else
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

			SDL_RenderDrawRect(renderer, currentSprite->GetRect());
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}		
	}
}

void Entity::SetSprite(Sprite* sprite)
{
	currentSprite = sprite;
}
