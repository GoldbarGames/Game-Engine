#pragma once
#include <unordered_map>
#include "SpriteManager.h"
#include "Sprite.h"
#include <SDL.h>

class Entity;

class Animator
{
private:
	std::unordered_map<std::string, Sprite*> mapStateToSprite;

	// parameters for triggering transitions between states
	std::unordered_map<std::string, bool> mapParamsBool;
	std::unordered_map<std::string, float> mapParamsFloat;
	std::unordered_map<std::string, int> mapParamsInt;
public:
	std::string animatorType = "";
	std::string currentState = "";
	std::string previousState = "";
	std::string beforePreviousState = "";
	int speed = 100;
	void SetState(std::string state);
	void OnEnter(std::string state);
	void DoState(Entity* entity);
	void OnExit(std::string state);
	void Update(Entity* entity);
	void SetBool(std::string param, bool value);
	void SetFloat(std::string param, float value);
	void SetInt(std::string param, int value);
	bool GetBool(std::string param);
	void MapStateToSprite(std::string state, Sprite* sprite);
	Animator(std::vector<Sprite*> sprites);
	Animator(std::string animType, std::string initialState);
	Animator(SpriteManager * spriteManager, SDL_Renderer * renderer);
	~Animator();

	void CheckStateKaneko();
};

