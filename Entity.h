#pragma once
#include "SDL.h"
#include "Sprite.h"
#include "Animator.h"
#include "Editor.h"

class Game;

//TODO: Make this work without overriding the class names
//enum EntityType { Player, Tile, Object };

class Entity
{
protected:
	Vector2 position = Vector2(0, 0);
	Animator* animator = nullptr;
	Sprite* currentSprite = nullptr;

public:
	std::string etype = "entity";
	int id = 0; //TODO
	int drawOrder = 0; // order for drawing
	int tilesheetIndex = 0;
	Vector2 tileCoordinates = Vector2(0, 0);
	Entity();
	~Entity();
	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	Animator* GetAnimator();
	virtual const SDL_Rect* GetBounds();
	Vector2 GetPosition();
	void SetPosition(Vector2 newPosition);
	void SetAnimator(Animator* anim);
	void SetSprite(Sprite* sprite);
	virtual void Update(Game& game);
	virtual void Render(SDL_Renderer * renderer, Vector2 cameraOffset);

	DrawingLayer layer = FOREGROUND;
};

