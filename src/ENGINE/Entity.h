#ifndef ENTITY_H
#define ENTITY_H

#include "leak_check.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "Vector2.h"
#include "globals.h"
#include "Animator.h"
#include "Collider.h"
#include "Sprite.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Game;
class Property;
class Renderer;
class QuadTree;
class Switch;

#ifndef STRUCT_FONT_INFO
#define STRUCT_FONT_INFO
class FontInfo;
#endif

class KINJO_API Entity
{
protected:	
	Animator* animator = nullptr;
	Sprite currentSprite;
public:
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 startPosition = glm::vec3(0, 0, 0);
	glm::vec3 lastPosition = glm::vec3(0, 0, 0);
	glm::vec3 rotation = glm::vec3(0.0f, 0.0f, 0.0f);
	Vector2 scale = Vector2(1, 1);
	
	Entity* attachedSwitch = nullptr;
	QuadTree* quadrant = nullptr;
	
	Color color = { 255, 255, 255, 255 };

	static uint32_t nextValidID;
	static std::unordered_map<uint32_t, bool> takenIDs;

	std::string name = "";
	std::string etype = "entity";

	// Since the sprite changes all the time due to animations,
	// we must have a single reference point for the shader
	//ShaderProgram* shader = nullptr;

	bool active = true;

	uint32_t id = 0;
	int subtype = 0;
	int drawOrder = 0; // order for drawing
	DrawingLayer layer = DrawingLayer::FRONT;	

	Collider collider;

	//TODO: Make multiple collision layers rather than just on/off

	//int collisionLayerID = 0;
	bool impassable = false; 
	bool trigger = false;	
	bool jumpThru = false;
	bool drawDebugRect = true;
	bool shouldDelete = false;
	bool shouldSave = false;
	bool clickable = false;
	bool draggable = false;

	bool isLightSource = false;

	unsigned int Size();

	static uint32_t GenerateValidID();

	virtual ~Entity();
	Entity(const glm::vec3& pos);
	Entity(const glm::vec3& pos, Sprite* sprite);

	Vector2 GetScale() const;

	virtual void SetScale(const Vector2& newScale);

	Sprite* GetSprite();
	Animator* GetAnimator();

	virtual const SDL_Rect* GetBounds();
	SDL_Rect GetTopLeftBounds() const;
	glm::vec3 GetPosition() const;
	glm::vec3 GetCenter() const;
	
	void SetPosition(const glm::vec3& newPosition);
	void SetAnimator(Animator& anim);
	void SetSprite(Sprite& sprite);

	virtual void Update(Game& game);
	virtual void Render(const Renderer& renderer);
	virtual void RenderParallax(const Renderer& renderer, float p);
	virtual void RenderDebug(const Renderer& renderer);

	void CreateCollider(float x, float y, float w, float h);
	void CalculateCollider();

	virtual void Init(const Game& g, const std::string& n);

	virtual void Pause(uint32_t ticks);
	virtual void Unpause(uint32_t ticks);

	virtual bool CanSpawnHere(const glm::vec3& spawnPosition, const Game& game, bool useCamera = true);

	// TODO: Refactor these functions into components

	virtual void OnTriggerStay(Entity& other, Game& game);
	virtual void OnTriggerEnter(Entity& other, Game& game);
	virtual void OnTriggerExit(Entity& other, Game& game);

	virtual void OnClick(Uint32 mouseState, Game& game);
	virtual void OnClickPressed(Uint32 mouseState, Game& game);
	virtual void OnClickReleased(Uint32 mouseState, Game& game);

	virtual void GetProperties(std::vector<Property*>& properties);
	void DeleteProperties(std::vector<Property*>& properties);
	void SetProperty(const std::string& key, const std::string& newValue, std::vector<Property*>& properties);

	virtual void Save(std::unordered_map<std::string, std::string>& map);
	virtual void Load(std::unordered_map<std::string, std::string>& map, Game& game);

	virtual void SetColor(Color newColor);

	virtual int GetGridSize();

	static Entity* __stdcall Create(const glm::vec3& pos) { return new Entity(pos); };
};

#endif 