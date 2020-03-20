#pragma once
#include <unordered_map>
#include "SpriteManager.h"
#include "Sprite.h"
#include "Timer.h"
#include <SDL.h>

class Entity;

struct AnimState {
	std::string name = "";
	int speed = 100;
	Sprite* sprite;

	AnimState()
	{

	}

	AnimState(std::string n, int s, Sprite* p)
	{
		name = n;
		speed = s;
		sprite = p;
	}
};

class Animator
{
private:
	//std::unordered_map<std::string, Sprite*> mapStateToSprite;

	// parameters for triggering transitions between states
	std::unordered_map<std::string, bool> mapParamsBool;
	std::unordered_map<std::string, float> mapParamsFloat;
	std::unordered_map<std::string, int> mapParamsInt;

	std::unordered_map<std::string, AnimState*> mapNamesToStates;
	
public:
	Sprite* GetCurrentSprite();
	Timer animationTimer;
	std::string animatorType = "";
	AnimState* currentState;
	AnimState* previousState;
	AnimState* beforePreviousState;
	void SetState(const char* state);
	void OnEnter(AnimState state);
	void DoState(Entity* entity);
	void OnExit(AnimState state);
	void Update(Entity* entity);
	void SetBool(const char* param, bool value);
	void SetFloat(const char* param, float value);
	void SetInt(const char* param, int value);
	bool GetBool(const char* param);
	void StartTimer();
	void MapStateNameToState(std::string name, AnimState* state);
	Animator(std::vector<Sprite*> sprites);
	Animator(std::string animType, std::vector<AnimState*> states, std::string initialState = "");
	Animator(SpriteManager * spriteManager, SDL_Renderer * renderer);
	~Animator();

	int GetSpeed();

	void CheckStateKaneko();
	void StateKanekoDebugSpell();
	bool IsStateDebugSpell();
};

