#pragma once
#include "SDL.h"
#include "Sprite.h"
#include "Animator.h"

class Game;

class Entity
{
protected:
	Animator* animator = nullptr;
	Sprite* currentSprite = nullptr;

	Vector2 position = Vector2(0, 0);
	Vector2 velocity = Vector2(0, 0);
	Vector2 acceleration = Vector2(0, 0);

	float horizontalSpeed = 0.001f;

public:
	Entity();
	~Entity();
	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	Animator* GetAnimator();
	const SDL_Rect* GetBounds();
	void SetPosition(Vector2 newPosition);
	void SetAnimator(Animator* anim);
	void SetSprite(Sprite* sprite);
	virtual void Update(Game& game);
	void Render(SDL_Renderer * renderer);
};

