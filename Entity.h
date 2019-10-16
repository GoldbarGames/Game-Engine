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



public:
	virtual ~Entity();
	Entity(Vector2 pos);

	std::string name = "";



	SDL_RendererFlip flip = SDL_FLIP_NONE;
	Vector2 entityPivot = Vector2(0, 0);
	bool shouldDelete = false;
	std::string etype = "entity";
	int id = 0; //TODO
	int drawOrder = 0; // order for drawing
	DrawingLayer layer = FRONT;

	// maybe move this to the Tile class
	int tilesheetIndex = 0;
	Vector2 tileCoordinates = Vector2(0, 0);


	SDL_Rect* collisionBounds = nullptr; // do not touch this until render time
	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	bool trigger = false;

	Sprite* GetSprite();
	Animator* GetAnimator();
	virtual const SDL_Rect* GetBounds();
	Vector2 GetPosition();
	Vector2 GetCenter();
	void SetPosition(Vector2 newPosition);
	void SetAnimator(Animator* anim);
	void SetSprite(Sprite* sprite);
	virtual void Update(Game& game);
	virtual void Render(Renderer * renderer, Vector2 cameraOffset);
	void RenderDebug(Renderer * renderer, Vector2 cameraOffset);

	virtual void Pause(Uint32 ticks);
	virtual void Unpause(Uint32 ticks);

	virtual bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);

	virtual void OnTriggerStay(Entity* other, Game& game);
	virtual void OnTriggerEnter(Entity* other, Game& game);
	virtual void OnTriggerExit(Entity* other, Game& game);

	virtual void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Text*>& properties);
	void DeleteProperties(std::vector<Text*>& properties);
	virtual void SetProperty(std::string prop, std::string newValue);
};

