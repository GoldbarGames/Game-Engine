#pragma once
#include "SDL.h"
#include "Sprite.h"
#include "Animator.h"

class Game;

class Entity
{
protected:
	Vector2 position = Vector2(0, 0);
	Animator* animator = nullptr;
	Sprite* currentSprite = nullptr;

public:
	int id = 0; //TODO
	int drawOrder = 0; // order for drawing
	Entity();
	~Entity();
	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	Animator* GetAnimator();
	virtual const SDL_Rect* GetBounds();
	void SetPosition(Vector2 newPosition);
	void SetAnimator(Animator* anim);
	void SetSprite(Sprite* sprite);
	virtual void Update(Game& game);
	virtual void Render(SDL_Renderer * renderer);
};

