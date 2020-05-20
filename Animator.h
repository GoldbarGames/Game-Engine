#pragma once
#include <unordered_map>
#include "SpriteManager.h"
#include "Sprite.h"
#include "Timer.h"
#include <SDL.h>

class Entity;

// For the sprite to know what to draw
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

struct AnimCondition
{
	std::string nextState;
	std::string variable;         // name
	std::string check;            // ==
	bool expectedValue;    // false

	AnimCondition(const std::string n, const std::string& v, const std::string& c, bool e)
	{
		nextState = n;
		variable = v;
		check = c;
		expectedValue = e;
	}
};

class AnimStateMachine
{
public:
	// state we are going to, conditions to get there
	std::unordered_map <std::string, std::vector<AnimCondition*>> conditions;
	//TODO: Currently because this is a map, we can't OR together any conditions
	// (which we could do by having them on a separate line)
	// so we need to either use a vector, or a different way
};

// For the animator parser to know how the states interact with each other
class AnimatorInfo 
{
public:
	//TODO: Store a way to represent the conditions for moving between states
	std::unordered_map<std::string, AnimStateMachine*> states;
	std::unordered_map<std::string, unsigned int> mapStateNamesToNumbers;
	std::unordered_map<std::string, unsigned int> mapKeysBool;
	std::unordered_map<std::string, unsigned int> mapKeysFloat;
	std::unordered_map<std::string, unsigned int> mapKeysInt;

	AnimatorInfo(std::string name);
};

enum class AnimType { None, Player, Block, Bug, DebugMissile, Door, Ether, Ladder, Platform, NPC, Shroom };

class Animator
{
private:
	//std::unordered_map<std::string, Sprite*> mapStateToSprite;

	// These are the keys for the below maps
	static std::unordered_map<AnimType, AnimatorInfo*> mapTypeToInfo;

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
	AnimType animatorType = AnimType::None;
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

	Animator(std::vector<Sprite*> sprites);
	Animator(AnimType animType, std::vector<AnimState*> states, std::string initialState = "");
	~Animator();

	int GetSpeed();

	void CheckStateKaneko();
	void StateKanekoDebugSpell();
	bool IsStateDebugSpell();
};

