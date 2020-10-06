#include "Animator.h"
#include "Entity.h"
#include "globals.h"
#include <utility>
#include <fstream>
#include "Sprite.h"
#include "Timer.h"

std::map<unsigned int, AnimatorInfo*> Animator::mapTypeToInfo;
std::unordered_map<std::string, unsigned int> Animator::mapNamesToAnimType;

AnimatorInfo::AnimatorInfo(std::string filePath)
{
	std::ifstream fin;

	//TODO: Customize prefix/suffix
	std::string animatorFile = "data/animators/" + filePath + ".animator";

	//TODO: Deal with issues involving extra whitespace (it breaks things)
	std::vector<std::string> stateNames;
	mapStateNamesToNumbers[""] = 0;
	states[""] = neww AnimStateMachine();

	bool readingInConditions = false;
	// Read in the state machine animator file
	std::cout << "Reading animator file:" << std::endl;
	fin.open(animatorFile);
	if (fin.is_open())
	{
		// We need to keep track of, for each line:
		// the state to go to
		// conditions to check
		// and for each condition:
		// 
		// variable type
		// variable name
		// check type (=, <, >)
		// expected value

		std::string stateName = "";
		int stateIndex = 1;
		int variableIndex = 0;
		int index = 0;

		std::vector<AnimCondition*> conditions;
		std::string variableType = "";
		std::string variableName = "";
		std::string conditionCheck = "";
		std::string expectedValue = "";
		std::string nextStateName = "";

		for (std::string line; std::getline(fin, line); )
		{
			if (line.size() == 0)
				continue;

			// remove trailing spaces
			while (!line.empty() && line.back() == ' ') 
				line.pop_back();

			if (line.front() == '*') // entering a neww state
			{ 
				if (readingInConditions)
				{
					stateNames.clear();
					readingInConditions = false;
				}

				index = 1;
				stateName = ParseWord(line, '*', index);
				stateNames.push_back(stateName);
				mapStateNamesToNumbers[stateName] = stateIndex++;
			}
			else
			{				
				readingInConditions = true;
				bool readLine = true;
				index = 0;

				nextStateName = ParseWord(line, ':', index);
				index++;

				conditions.clear();

				while (readLine)
				{
					variableType = ParseWord(line, ' ', index);
					variableName = ParseWord(line, ' ', index);
					conditionCheck = ParseWord(line, ' ', index);
					expectedValue = ParseWord(line, ' ', index);

					//TODO: If we really want to do this the right way
					// we just need to use a binary search tree (abstract syntax tree)

					readLine = (ParseWord(line, ' ', index) == "&&");

					if (variableType == "bool")
					{
						mapKeysBool[variableName] = variableIndex++;
					}
					else if (variableType == "float")
					{
						mapKeysFloat[variableName] = variableIndex++;
					}
					else if (variableType == "int")
					{
						mapKeysInt[variableName] = variableIndex++;
					}

					// Add this condition to the list of conditions for this state
					//TODO: Make sure this does not cause a memory leak
					//states[stateName]->conditions
						
					conditions.push_back(new AnimCondition(nextStateName, variableName,
						conditionCheck, (expectedValue == "true")));

					// state name associated with a vector of structs (conditions)
				}

				// after parsing all conditions, assign them to the state(s)

				for (int i = 0; i < stateNames.size(); i++)
				{
					if (states.count(stateNames[i]) != 1)
					{
						states[stateNames[i]] = neww AnimStateMachine();
					}
					states[stateNames[i]]->conditions[nextStateName] = conditions;
				}
			}

			
		}
	}

	fin.close();
}



Animator::Animator(std::vector<Sprite*> sprites)
{

}

unsigned int Animator::GetNumberOfStateFromName(const char* name)
{
	return 0;
}

// PRE-CONDITION: The list of states is not empty
Animator::Animator(const std::string& filePath, std::vector<AnimState*> states, std::string initialState)
{
	MapStateNameToState("", neww AnimState("", 0, nullptr));

	if (mapNamesToAnimType.count(filePath) != 1)
	{
		if (mapNamesToAnimType.size() > 0)
		{
			mapNamesToAnimType[filePath] = mapNamesToAnimType.size() + 1;
			//mapNamesToAnimType[filePath] = (--mapNamesToAnimType.end())->second + 1;
		}
		else
		{
			mapNamesToAnimType[filePath] = 0;
		}		
	}

	animatorType = mapNamesToAnimType[filePath];

	// If this animator type has not been initialized, do so here
	if (mapTypeToInfo.count(animatorType) != 1)
	{
		// Parse the animator state info here
		mapTypeToInfo[animatorType] = neww AnimatorInfo(filePath);
	}

	// Save the vector of states as a map
	for (unsigned int i = 0; i < states.size(); i++)
	{
		MapStateNameToState(states[i]->name, states[i]);
	}

	// Set the initial state
	if (initialState != "")
	{
		bool found = false;
		for (unsigned int i = 0; i < states.size(); i++)
		{
			if (states[i]->name == initialState)
			{
				found = true;
				SetState(states[i]->name.c_str());
				break;
			}
		}

		if (!found)
			SetState(states[0]->name.c_str());
	}
	else
	{
		SetState(states[0]->name.c_str());
	}
}

Animator::~Animator()
{

}

void Animator::MapStateNameToState(const std::string& name, AnimState* state)
{
	mapNamesToStates[name] = state;
}

AnimState* Animator::GetState(const std::string& name)
{
	return mapNamesToStates[name];
}

void Animator::OnEnter(AnimState state)
{

}

void Animator::DoState(Entity& entity)
{
	entity.SetSprite(*GetCurrentSprite());
}

void Animator::OnExit(AnimState state)
{

}

void Animator::Update(Entity& entity)
{
	if (entity.etype == "switch")
		int test = 0;

	// If conditions met, set current state to next state
	// Else, stay in current state

	AnimatorInfo* info = mapTypeToInfo[animatorType];
	AnimStateMachine* stateMachine = info->states[currentState->name];

	//TODO: Make sure to run through each condition, and if the whole expression is true,
	// then we go to the first state that has the whole expression true

	if (stateMachine != nullptr)
	{
		bool allConditionsTrue = false;
		AnimCondition* condition = nullptr;

		// first = string = name of the state to go to
		// second = vector<AnimCondition*> = list of conditions to fulfill
		for (auto const& nextState : stateMachine->conditions)
		{
			allConditionsTrue = true;
			std::vector<AnimCondition*> conditions = nextState.second;
			for (int i = 0; i < conditions.size(); i++)
			{
				condition = conditions[i];
				if (condition->check == "==")
				{
					if (mapParamsBool[info->mapKeysBool[condition->variable]] == condition->expectedValue)
					{
						// do nothing
					}
					else
					{
						allConditionsTrue = false;
					}
				}
				else
				{
					std::cout << "ERROR: Check " << condition->check << std::endl;
				}
			}

			if (allConditionsTrue) // then go to the next state
			{
				
				if (condition != nullptr && currentState->name != condition->nextState.c_str())
				{
					if (currentState->name == "debug")
						int test = 0;
					if (currentState->name == "idle")
						int test = 0;
				}

				if (entity.etype == "player")
					std::cout << condition->nextState.c_str() << std::endl;

				SetState(condition->nextState.c_str());
				DoState(entity);
				return; // set state to the first one that matches
			}
		}
	}	

	// Then, carry out whatever the current state is
	// TODO: Double-check this, should we do this before returning above?
	DoState(entity);
}

//TODO: Refactor all of this so that:
// 1. Use uint IDs instead of names for states and variables
// 2. Read in file 1 to create a map of strings (states/vars) to ints
// 3. Read in file 2 and parse it like a state machine
// (rather than hard-coding each state machine)

//TODO: The IDs and parsing could probably be static, 
// or done by a singleton so that each entity
// does not need to have the parsing logic in it

void Animator::StartTimer()
{
	// set duration of the animation based on the playback speed and number of frames
	//animationTimer.Start(currentState->speed * GetCurrentSprite()->endFrame, GetCurrentSprite()->shouldLoop);
}

const AnimState& Animator::GetCurrentState()
{
	return *currentState;
}

Sprite* Animator::GetCurrentSprite()
{
	return currentState->sprite;
}

int Animator::GetSpeed()
{
	return currentState->speed;
}

void Animator::SetScaleAllStates(const Vector2& newScale)
{
	for (auto const& [key, val] : mapNamesToStates)
	{
		if (val->sprite != nullptr)
		{
			val->sprite->SetScale(newScale);
		}
	}
}

void  Animator::SetRelativeAllStates(bool b)
{
	for (auto const& [key, val] : mapNamesToStates)
	{
		if (val->sprite != nullptr)
		{
			val->sprite->keepPositionRelativeToCamera = b;
			val->sprite->keepScaleRelativeToCamera = b;
		}
	}
}

void Animator::SetState(const char* state)
{
	currentState = mapNamesToStates[state];

	// TODO: If we're going into the same state, don't reset the current frame
	//if (previousState != nullptr && previousState->name == state)
		
	StartTimer();
}

bool Animator::GetBool(const char* param)
{
	// NOTE: It's important to check if the variable exists,
	// otherwise it will store the value as 0 during lookup.
	// And that would ruin our results later on.

	if (mapTypeToInfo[animatorType]->mapKeysBool.count(param) == 0)
	{
		mapTypeToInfo[animatorType]->mapKeysBool[param] = mapTypeToInfo[animatorType]->mapKeysBool.size() + 2;
	}

	return mapParamsBool[mapTypeToInfo[animatorType]->mapKeysBool[param]];
}

float Animator::GetFloat(const char* param)
{
	if (mapTypeToInfo[animatorType]->mapKeysFloat.count(param) == 0)
	{
		mapTypeToInfo[animatorType]->mapKeysFloat[param] = mapTypeToInfo[animatorType]->mapKeysFloat.size() + 2;
	}

	return mapParamsFloat[mapTypeToInfo[animatorType]->mapKeysFloat[param]];
}

int Animator::GetInt(const char* param)
{
	if (mapTypeToInfo[animatorType]->mapKeysInt.count(param) == 0)
	{
		mapTypeToInfo[animatorType]->mapKeysInt[param] = mapTypeToInfo[animatorType]->mapKeysInt.size() + 2;
	}

	return mapParamsInt[mapTypeToInfo[animatorType]->mapKeysInt[param]];
}

void Animator::SetBool(const char* param, bool value)
{
	// TODO: Extract these out to a generic function (CheckKeyExists)

	if (mapTypeToInfo[animatorType]->mapKeysBool.count(param) == 0)
	{
		mapTypeToInfo[animatorType]->mapKeysBool[param] = mapTypeToInfo[animatorType]->mapKeysBool.size() + 2;
	}

	mapParamsBool[mapTypeToInfo[animatorType]->mapKeysBool[param]] = value;
}

void Animator::SetFloat(const char* param, float value)
{
	if (mapTypeToInfo[animatorType]->mapKeysFloat.count(param) == 0)
	{
		mapTypeToInfo[animatorType]->mapKeysFloat[param] = mapTypeToInfo[animatorType]->mapKeysFloat.size() + 2;
	}

	mapParamsFloat[mapTypeToInfo[animatorType]->mapKeysFloat[param]] = value;
}

void Animator::SetInt(const char* param, int value)
{
	if (mapTypeToInfo[animatorType]->mapKeysInt.count(param) == 0)
	{
		mapTypeToInfo[animatorType]->mapKeysInt[param] = mapTypeToInfo[animatorType]->mapKeysInt.size() + 2;
	}

	mapParamsInt[mapTypeToInfo[animatorType]->mapKeysInt[param]] = value;
}