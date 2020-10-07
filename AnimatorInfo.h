#ifndef ANIMATORINFO_H
#define ANIMATORINFO_H
#pragma once

#include <unordered_map>
#include "Sprite.h"

// For the sprite to know what to draw
struct AnimState {
	std::string name = "";
	int speed = 100;
	Sprite* sprite = nullptr;

	AnimState() { }

	AnimState(std::string n, int s, Sprite* p) : name(n), speed(s), sprite(p) { }
};

struct AnimCondition
{
	std::string nextState = "";
	std::string variable = "";         // name
	std::string check = "";            // ==
	bool expectedValue = false;        // false

	AnimCondition(const std::string n, const std::string& v, const std::string& c, bool e)
		: nextState(n), variable(v), check(c), expectedValue(e) { }

	~AnimCondition() {}
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
	std::unordered_map<std::string, AnimStateMachine*> states;
	std::unordered_map<std::string, unsigned int> mapStateNamesToNumbers;
	std::unordered_map<std::string, unsigned int> mapKeysBool;
	std::unordered_map<std::string, unsigned int> mapKeysFloat;
	std::unordered_map<std::string, unsigned int> mapKeysInt;

	AnimatorInfo(const std::string& filePath);
	~AnimatorInfo();
};

#endif