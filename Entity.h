#pragma once
#include "SDL.h"
#include "Sprite.h"
#include "Animator.h"
#include "Editor.h"

class Game;

class Entity
{
protected:
	static unsigned int nextValidID;
	Vector2 position = Vector2(0, 0);
	Animator* animator = nullptr;
	Sprite* currentSprite = nullptr;

	Entity(Vector2 pos);

public:
	virtual ~Entity();
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	Vector2 entityPivot = Vector2(0, 0);
	bool shouldDelete = false;
	std::string etype = "entity";
	int id = 0; //TODO
	int drawOrder = 0; // order for drawing
	DrawingLayer layer = FOREGROUND;

	// maybe move this to the Tile class
	int tilesheetIndex = 0;
	Vector2 tileCoordinates = Vector2(0, 0);

	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	Sprite* GetSprite();
	Animator* GetAnimator();
	virtual const SDL_Rect* GetBounds();
	Vector2 GetPosition();
	Vector2 GetCenter();
	void SetPosition(Vector2 newPosition);
	void SetAnimator(Animator* anim);
	void SetSprite(Sprite* sprite);
	virtual void Update(Game& game);
	virtual void Render(SDL_Renderer * renderer, Vector2 cameraOffset);

	virtual void Pause(Uint32 ticks);
	virtual void Unpause(Uint32 ticks);

	virtual bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);
};

