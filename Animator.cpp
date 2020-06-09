#include "Animator.h"
#include "Entity.h"
#include "globals.h"
#include <utility>
#include <fstream>

std::unordered_map<AnimType, AnimatorInfo*> Animator::mapTypeToInfo;

AnimatorInfo::AnimatorInfo(std::string name)
{
	std::ifstream fin;

	//TODO: Refactor this so that you can have custom paths for these files
	std::string animatorFile = "data/animators/" + name + "/" + name + ".animator";
	std::string statesFile = "data/animators/" + name + "/" + name + ".states";
	std::string varsFile = "data/animators/" + name + "/" + name + ".vars";

	//TODO: Deal with issues involving extra whitespace (it breaks things)

	// Read in all the states for this animator type
	//TODO: These can probably be inferred from the state machine

	std::cout << "Reading states file:" << std::endl;
	fin.open(statesFile);
	if (fin.is_open())
	{
		mapStateNamesToNumbers[""] = 0;
		int i = 1;
		for (std::string line; std::getline(fin, line); )
		{
			mapStateNamesToNumbers[line] = i;
			i++;
		}
	}
	fin.close();

	// Read in all the variables for this animator type
	//TODO: These can probably be inferred from the state machine
	std::cout << "Reading vars file:" << std::endl;
	fin.open(varsFile);
	if (fin.is_open())
	{
		int i = 1;
		for (std::string line; std::getline(fin, line); )
		{
			int index = 0;
			std::string variableType = ParseWord(line, ' ', index);
			std::string variableName = ParseWord(line, ' ', index);

			if (variableType == "bool")
			{
				mapKeysBool[variableName] = i;
			}
			else if (variableType == "float")
			{
				mapKeysBool[variableName] = i;
			}
			else if (variableType == "int")
			{
				mapKeysBool[variableName] = i;
			}
			
			i++;
		}
	}
	fin.close();

	states[""] = new AnimStateMachine();

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

		for (std::string line; std::getline(fin, line); )
		{
			if (line.size() == 0)
				continue;

			// remove trailing spaces
			while (!line.empty() && line.back() == ' ') 
				line.pop_back();

			if (line.front() == '*') // entering a new state
			{ 
				if (stateName != "")
				{
					//TODO: Save info for this state
					// before moving on to the next one
				}

				int index = 1;
				stateName = ParseWord(line, '*', index);
			}
			else
			{
				int index = 0;
				bool readLine = true;

				std::string nextStateName = ParseWord(line, ':', index);
				index++;

				std::vector<AnimCondition*> conditions;

				while (readLine)
				{
					std::string variableType = ParseWord(line, ' ', index);
					std::string variableName = ParseWord(line, ' ', index);
					std::string conditionCheck = ParseWord(line, ' ', index);
					std::string expectedValue = ParseWord(line, ' ', index);

					//TODO: If we really want to do this the right way
					// we just need to use a binary search tree (abstract syntax tree)

					std::string endOfLine = ParseWord(line, ' ', index);

					readLine = (endOfLine == "&&");

					if (states.count(stateName) != 1)
					{
						states[stateName] = new AnimStateMachine();
					}

					// Add this condition to the list of conditions for this state
					//TODO: Make sure this does not cause a memory leak
					//states[stateName]->conditions
						
					conditions.push_back(new AnimCondition(nextStateName, variableName,
						conditionCheck, (expectedValue == "true")));

					// state name associated with a vector of structs (conditions)
				}

				// after parsing all conditions, assign them to the next state
				states[stateName]->conditions[nextStateName] = conditions;				
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
Animator::Animator(AnimType animType, std::vector<AnimState*> states, std::string initialState)
{
	MapStateNameToState("", new AnimState("", 0, nullptr));

	// If this animator type has not been initalized, do so here
	if (mapTypeToInfo.count(animType) != 1)
	{
		// Parse the animator state info here
		//TODO: Maybe change from enums to ints, and map these ints in an external file,
		// that way we don't have to add any enums for new animations
		//TODO: We want to map the animType to the vars/states and parse its file here
		switch (animType)
		{
		case AnimType::Player:
			mapTypeToInfo[animType] = new AnimatorInfo("player");
			break;
		case AnimType::Cursor:
			mapTypeToInfo[animType] = new AnimatorInfo("cursor");
			break;
		default:
			mapTypeToInfo[animType] = new AnimatorInfo("");
			break;
		}
		
	}

	animatorType = animType;

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

	previousState = currentState;
	beforePreviousState = previousState;
}

Animator::~Animator()
{

}

void Animator::MapNumbersToState(unsigned int number, AnimState* state)
{
	mapNumbersToStates[number] = state;
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

void Animator::DoState(Entity* entity)
{
	entity->SetSprite(GetCurrentSprite());
}

void Animator::OnExit(AnimState state)
{

}

void Animator::Update(Entity* entity)
{
	//TODO: How to check if the animation has played exactly once?

	// If conditions met, set current state to next state
	// Else, stay in current state

	AnimatorInfo* info = mapTypeToInfo[animatorType];
	AnimStateMachine* stateMachine = mapTypeToInfo[animatorType]->states[currentState->name];

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
					int test = 0;
				}

				SetState(condition->nextState.c_str());
				return; // set state to the first one that matches
			}
		}

 

	}	

	/*
	else if (animatorType == "debug_missile")
	{
		if (currentState->name == "moving")
		{
			if (GetBool("destroyed"))
				SetState("destroyed");
		}
	}
	else if (animatorType == "door")
	{
		if (currentState->name == "closed")
		{
			if (GetBool("opened"))
				SetState("opened");
		}
	}
	*/

	// Then, carry out whatever the current state is
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
	animationTimer.Start(currentState->speed * GetCurrentSprite()->endFrame,
		GetCurrentSprite()->shouldLoop);
}

Sprite* Animator::GetCurrentSprite()
{
	return currentState->sprite;
}

int Animator::GetSpeed()
{
	return currentState->speed;
}

void Animator::SetScaleAllStates(Vector2 newScale)
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
	beforePreviousState = previousState;
	previousState = currentState;	
	currentState = mapNamesToStates[state];
	currentState->sprite->currentFrame = 0;

	StartTimer();
}

bool Animator::GetBool(const char* param)
{
	//TODO: Check if it exists first
	return mapParamsBool[mapTypeToInfo[animatorType]->mapKeysBool[param]];
}

void Animator::SetBool(const char* param, bool value)
{
	mapParamsBool[mapTypeToInfo[animatorType]->mapKeysBool[param]] = value;
}

void Animator::SetFloat(const char* param, float value)
{
	mapParamsFloat[mapTypeToInfo[animatorType]->mapKeysFloat[param]] = value;
}

void Animator::SetInt(const char* param, int value)
{
	mapParamsInt[mapTypeToInfo[animatorType]->mapKeysInt[param]] = value;
}