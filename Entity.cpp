#include "Entity.h"

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

void Entity::SetPosition(Vector2 newPosition)
{
	position = newPosition;
}

void Entity::SetAnimator(Animator * anim)
{
	animator = anim;
}

void Entity::Render(SDL_Renderer * renderer)
{
	if (currentSprite != nullptr)
	{
		if (animator != nullptr)
			currentSprite->Render(position, animator->speed, renderer);
		else
			currentSprite->Render(position, 0, renderer);
	}
}

void Entity::SetSprite(Sprite* sprite)
{
	currentSprite = sprite;
}
