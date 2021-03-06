#ifndef ANIMATOR_H
#define ANIMATOR_H
#pragma once

#include "leak_check.h"
#include <map>
#include <unordered_map>
#include <SDL2/SDL.h>

#include "Timer.h"
#include "AnimatorInfo.h"

class Entity;
class Sprite;
class Vector2;
class SpriteManager;

class KINJO_API Animator
{
private:
	//std::unordered_map<std::string, Sprite*> mapStateToSprite;

	// These are the keys for the below maps
	static std::unordered_map<std::string, unsigned int> mapNamesToAnimType;


	// parameters for triggering transitions between states
	std::unordered_map<unsigned int, bool> mapParamsBool;
	std::unordered_map<unsigned int, float> mapParamsFloat;
	std::unordered_map<unsigned int, int> mapParamsInt;

	std::unordered_map<std::string, AnimState*> animStates;
public:
	static SpriteManager* spriteManager;
	static std::map<unsigned int, AnimatorInfo*> mapTypeToInfo;
	Sprite* GetCurrentSprite();

	Timer animationTimer;
	int animatorType = 0;
	AnimState* currentState = nullptr;
	bool shouldUpdate = true;

	void SetState(const char* state);
	void OnEnter(AnimState state);
	void DoState(Entity& entity);
	void OnExit(AnimState state);
	void Update(Entity& entity);

	void SetBool(const char* param, bool value);
	void SetFloat(const char* param, float value);
	void SetInt(const char* param, int value);
	bool GetBool(const char* param);
	float GetFloat(const char* param);
	int GetInt(const char* param);

	void MapStateNameToState(const std::string& name, AnimState* state);

	void SetSpriteFromState(AnimState* animState, Sprite& sprite);
	void SetSpriteFromState(const std::string& state, Sprite& sprite);

	unsigned int GetNumberOfStateFromName(const char* name);

	AnimState* GetState(const std::string& name);
	const AnimState& GetCurrentState();

	Animator(std::vector<Sprite*> sprites);
	Animator(const std::string& filePath, std::vector<AnimState*> states, std::string initialState = "");
	~Animator();

	int GetSpeed();
};

#endif