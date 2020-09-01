#ifndef ENTITY_H
#define ENTITY_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "Vector2.h"
#include "globals.h"
#include "Animator.h"
#include "Collider.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


class Game;
class PhysicsComponent;
class Property;
class Renderer;
class QuadTree;
class Switch;

#ifndef STRUCT_FONT_INFO
#define STRUCT_FONT_INFO
struct FontInfo;
#endif

class Entity
{
protected:	
	Animator* animator = nullptr;
	Sprite* currentSprite = nullptr;
	Sprite* debugSprite = nullptr;
public:
	Vector2 position = Vector2(0, 0);
	Vector2 lastPosition = Vector2(0, 0);
	glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	Vector2 scale = Vector2(1, 1);
	int spriteIndex = 0;

	Switch* attachedSwitch = nullptr;
	PhysicsComponent* physics = nullptr;
	bool clickable = false;
	QuadTree* quadrant = nullptr;

	unsigned int Size();

	static unsigned int GetNextValidID();
	static unsigned int nextValidID;
	
	virtual ~Entity();
	Entity(const Vector2& pos);
	Entity(const Vector2& pos, Sprite* sprite);
	
	bool drawDebugRect = true;

	std::string name = "";
	bool shouldDelete = false;
	std::string etype = "entity";
	int id = 0; //TODO
	int drawOrder = 0; // order for drawing
	DrawingLayer layer = DrawingLayer::FRONT;

	// maybe move this to the Tile class
	int tilesheetIndex = 0;
	Vector2 tileCoordinates = Vector2(0, 0);	

	Collider* collider = nullptr;
	SDL_Rect* bounds = nullptr;

	bool impassable = false; //TODO: Make multiple collision layers rather than just on/off
	bool trigger = false;	
	bool jumpThru = false;

	Sprite* GetSprite();
	Animator* GetAnimator();
	virtual const SDL_Rect* GetBounds();
	Vector2 GetPosition() const;
	Vector2 GetCenter() const;
	
	void SetPosition(const Vector2& newPosition);
	void SetAnimator(Animator& anim);
	void SetSprite(Sprite& sprite);

	virtual void Update(Game& game);
	virtual void Render(const Renderer& renderer);
	virtual void RenderParallax(const Renderer& renderer, float p);
	virtual void RenderDebug(const Renderer& renderer);

	void CreateCollider(float x, float y, float w, float h);
	void CalculateCollider();

	virtual void Pause(Uint32 ticks);
	virtual void Unpause(Uint32 ticks);

	virtual bool CanSpawnHere(const Vector2& spawnPosition, Game& game, bool useCamera = true);

	virtual void OnTriggerStay(Entity& other, Game& game);
	virtual void OnTriggerEnter(Entity& other, Game& game);
	virtual void OnTriggerExit(Entity& other, Game& game);

	virtual void OnClick(Uint32 mouseState, Game& game);
	virtual void OnClickPressed(Uint32 mouseState, Game& game);
	virtual void OnClickReleased(Uint32 mouseState, Game& game);

	virtual void GetProperties(FontInfo* font, std::vector<Property*>& properties);
	void DeleteProperties(std::vector<Property*>& properties);
	virtual void SetProperty(const std::string& key, const std::string& newValue);

	virtual void Save(std::ostringstream& level);

	virtual void SetColor(Color newColor);

	static Entity* __stdcall Create(const Vector2& pos) { return new Entity(pos); };
};

#endif 