#pragma once
#include "SDL.h"
#include "Sprite.h"
#include "Animator.h"
#include "Editor.h"

#include "Property.h"

#include "Mesh.h"
#include "Texture.h"
#include "Shader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Game;
class PhysicsEntity;

class Entity
{
protected:	
	Vector2 position = Vector2(0, 0);	
	Animator* animator = nullptr;
	Sprite* currentSprite = nullptr;
	float colliderWidth = 1;
	float colliderHeight = 1;

public:



	static unsigned int nextValidID;
	virtual ~Entity();
	Entity(Vector2 pos);
	Entity(Vector2 pos, Sprite* sprite);
	
	bool isPhysicsEntity = false;
	bool drawDebugRect = true;

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

	SDL_Rect* collider = nullptr;        // adjust the bounds this way
	SDL_Rect* collisionBounds = nullptr; // do not touch this until render time
	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	bool trigger = false;	
	bool jumpThru = false;

	Sprite* GetSprite();
	Animator* GetAnimator();
	virtual const SDL_Rect* GetBounds();
	Vector2 GetPosition();
	Vector2 GetCenter();
	void SetPosition(Vector2 newPosition);
	void SetAnimator(Animator* anim);
	void SetSprite(Sprite* sprite);
	virtual void Update(Game& game);
	virtual void Render(Renderer * renderer);
	virtual void Render(Renderer* renderer, Vector2 offset);
	virtual void RenderParallax(Renderer* renderer, float p);

	void RenderDebug(Renderer * renderer, Vector2 cameraOffset);

	void CreateCollider(float startX, float startY, float x, float y, float w, float h);
	void CalculateCollider(Vector2 cameraOffset);

	virtual void Pause(Uint32 ticks);
	virtual void Unpause(Uint32 ticks);

	virtual bool CanSpawnHere(Vector2 spawnPosition, Game& game, bool useCamera = true);

	virtual void OnTriggerStay(Entity* other, Game& game);
	virtual void OnTriggerEnter(Entity* other, Game& game);
	virtual void OnTriggerExit(Entity* other, Game& game);

	virtual void GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties);
	void DeleteProperties(std::vector<Property*>& properties);
	virtual void SetProperty(std::string prop, std::string newValue);

	virtual void Save(std::ostringstream& level);

	virtual bool IsEntityPushingOther(PhysicsEntity* other, bool x);
	virtual float CalcCollisionVelocity(PhysicsEntity* other, bool x);

};

