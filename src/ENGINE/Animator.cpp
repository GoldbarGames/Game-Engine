#include "Animator.h"
#include "Entity.h"
#include "globals.h"
#include <utility>
#include <fstream>
#include "Sprite.h"
#include "Timer.h"
#include "AnimatorInfo.h"
#include "SpriteManager.h"
#include "Renderer.h"
#include "Game.h"

std::map<unsigned int, AnimatorInfo*> Animator::mapTypeToInfo;
std::unordered_map<std::string, unsigned int> Animator::mapNamesToAnimType;
SpriteManager* Animator::spriteManager;

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
	//MapStateNameToState("", new AnimState("", 0, nullptr));

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
		mapTypeToInfo[animatorType] = new AnimatorInfo(filePath);
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
	/*
	for (auto& [key, val] : animStates)
	{
		if (val != nullptr)
			delete_it(val);
	}
	*/
}

void Animator::MapStateNameToState(const std::string& name, AnimState* state)
{
	animStates[name] = state;
}

AnimState* Animator::GetState(const std::string& name)
{
	return animStates[name];
}

void Animator::OnEnter(AnimState state)
{

}

void Animator::DoState(Entity& entity)
{
	SetSpriteFromState(currentState, *(entity.GetSprite()));
}

void Animator::SetSpriteFromState(AnimState* animState, Sprite& sprite)
{
	if (animState != nullptr && spriteManager != nullptr)
	{
		sprite.texture = spriteManager->GetImage(animState->filename);
		sprite.startFrame = animState->startFrame;
		sprite.endFrame = animState->endFrame;
		sprite.frameWidth = std::min(animState->frameWidth, sprite.texture->GetWidth());
		sprite.frameHeight = std::min(animState->frameHeight, sprite.texture->GetHeight());
		sprite.pivot = glm::vec2(animState->pivotX, animState->pivotY);

		if (sprite.currentFrame > sprite.endFrame)
		{
			sprite.currentFrame = sprite.startFrame;
			sprite.previousFrame = sprite.currentFrame;
		}

		sprite.playedOnce = false;

		//TODO: This only works if there is only one row, but that is okay for now
		sprite.numberFramesInTexture = sprite.texture->GetWidth() / sprite.frameWidth;
		sprite.framesPerRow = sprite.numberFramesInTexture;
	}
}

void Animator::SetSpriteFromState(const std::string& state, Sprite& sprite)
{
	SetSpriteFromState(GetState(state), sprite);
}

void Animator::OnExit(AnimState state)
{

}

void Animator::Update(Entity& entity)
{
	if (!shouldUpdate)
		return;

	// If conditions met, set current state to next state
	// Else, stay in current state

	AnimatorInfo* info = mapTypeToInfo[animatorType];
	AnimStateMachine* stateMachine = &(info->stateMachines[currentState->name]);

	//TODO: Make sure to run through each condition, and if the whole expression is true,
	// then we go to the first state that has the whole expression true

	if (stateMachine != nullptr)
	{
		bool allConditionsTrue = false;
		AnimCondition* condition = nullptr;

		// first = string = name of the state to go to
		// second = vector<AnimCondition*> = list of conditions to fulfill
		for (auto& nextState : stateMachine->conditions)
		{
			allConditionsTrue = true;
			for (size_t i = 0; i < nextState.second.size(); i++)
			{
				condition = &(nextState.second[i]);
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
				else if (condition->check == "timeout")
				{
					if (animationTimer.HasElapsed())
					{

					}
					else
					{
						allConditionsTrue = false;
					}
				}
				else if (condition->check == "gif")
				{
					if (spriteManager->renderer->game->savingGIF)
					{
						//allConditionsTrue = true;
					}
					else
					{
						//allConditionsTrue = false;
					}
				}
				else
				{
					std::cout << "ERROR: Check " << condition->check << std::endl;
				}
			}

			if (allConditionsTrue) // then go to the next state
			{
				//if (entity.etype == "player")
				//	std::cout << condition->nextState.c_str() << std::endl;

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

const AnimState& Animator::GetCurrentState()
{
	return *currentState;
}

Sprite* Animator::GetCurrentSprite()
{
	return nullptr; // currentState->sprite;
}

int Animator::GetSpeed()
{
	return currentState->speed;
}

void Animator::SetState(const char* state)
{
	currentState = animStates[state];

	// Always start the timer equal to the speed of the state
	animationTimer.Start(currentState->speed);

	// TODO: If we're going into the same state, don't reset the current frame
	//if (previousState != nullptr && previousState->name == state)
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