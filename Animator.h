#ifndef ANIMATOR_H
#define ANIMATOR_H
#pragma once

#include <map>
#include <unordered_map>
#include <SDL.h>

#include "Timer.h"

class Entity;

struct AnimState;
struct AnimCondition;
struct AnimStateMachine;
struct AnimatorInfo;

class Sprite;
class Vector2;

class Animator
{
private:
	//std::unordered_map<std::string, Sprite*> mapStateToSprite;

	// These are the keys for the below maps
	static std::map<unsigned int, AnimatorInfo*> mapTypeToInfo;
	static std::unordered_map<std::string, unsigned int> mapNamesToAnimType;

	// parameters for triggering transitions between states
	std::unordered_map<unsigned int, bool> mapParamsBool;
	std::unordered_map<unsigned int, float> mapParamsFloat;
	std::unordered_map<unsigned int, int> mapParamsInt;

	//TODO: Remove this?
	std::unordered_map<std::string, AnimState*> mapNamesToStates;	

	std::unordered_map<unsigned int, AnimState*> mapNumbersToStates;
public:


	Sprite* GetCurrentSprite();
	Timer animationTimer;
	int animatorType = 0;
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
	void MapStateNameToState(const std::string& name, AnimState* state);
	void MapNumbersToState(unsigned int number, AnimState* state);

	unsigned int GetNumberOfStateFromName(const char* name);

	void SetScaleAllStates(Vector2 newScale);
	void SetRelativeAllStates(bool b);

	AnimState* GetState(const std::string& name);

	Animator(std::vector<Sprite*> sprites);
	Animator(const std::string& entityName, std::vector<AnimState*> states, std::string initialState = "");
	~Animator();

	int GetSpeed();
};

#endif