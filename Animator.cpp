#include "Animator.h"
#include "Entity.h"
#include <utility>

Animator::Animator(std::vector<Sprite*> sprites)
{

}

// PRE-CONDITION: The list of states is not empty
Animator::Animator(std::string animType, std::vector<AnimState*> states, std::string initialState)
{
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

Animator::Animator(SpriteManager * spriteManager, SDL_Renderer * renderer)
{

}

Animator::~Animator()
{

}

void Animator::MapStateNameToState(std::string name, AnimState* state)
{
	mapNamesToStates[name] = state;
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

	if (animatorType == "kaneko")
	{
		CheckStateKaneko();
	}
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

	// Then, carry out whatever the current state is
	DoState(entity);
}

void Animator::CheckStateKaneko()
{
	if (currentState->name == "look_up")
	{
		if (!GetBool("holdingUp"))
		{
			SetState("idle");
		}
		else if (GetBool("isCastingDebug"))
		{
			if (GetBool("isGrounded"))
				SetState("debug_up");
			else
				SetState("debug_air_up");
		}
	}
	else if (currentState->name == "look_down")
	{
		if (!GetBool("holdingDown"))
		{
			SetState("idle");
		}
		else if (GetBool("isCastingDebug"))
		{
			if (GetBool("isGrounded"))
				SetState("debug_down");
			else
				SetState("debug_air_down");
		}
	}

	if (currentState->name == "walk")
	{
		if (!GetBool("walking"))
			SetState("idle");

		if (!GetBool("isGrounded"))
			SetState("jump");

		if (GetBool("isCastingDebug"))
		{
			if (GetBool("holdingUp"))
				SetState("debug_up");
			else if (GetBool("holdingDown"))
				SetState("debug_down");
			else
				SetState("debug");
		}
	}
	else if (currentState->name == "blink")
	{
		if (GetBool("walking"))
			SetState("walk");

		if (!GetBool("isGrounded"))
			SetState("jump");

		if (GetBool("isCastingDebug"))
		{
			if (GetBool("holdingUp"))
				SetState("debug_up");
			else if (GetBool("holdingDown"))
				SetState("debug_down");
			else
				SetState("debug");
		}
	}
	else if (currentState->name == "ladder_idle")
	{
		if (!GetBool("onLadder"))
		{
			if (GetBool("isGrounded"))
				SetState("idle");
			else
				SetState("jump");
		}
		else
		{
			if (GetBool("climbing"))
			{
				SetState("ladder_climbing");
			}
		}
	}
	else if (currentState->name == "ladder_climbing")
	{
		if (!GetBool("onLadder"))
		{
			if (GetBool("isGrounded"))
				SetState("idle");
			else
				SetState("jump");
		}
		else
		{
			if (!GetBool("climbing"))
			{
				SetState("ladder_idle");
			}
		}
	}
	else if (currentState->name == "idle")
	{
		if (GetBool("holdingUp"))
		{
			if (GetBool("onLadder"))
			{
				SetState("ladder_idle");
				return; 
			}				
			else
			{
				SetState("look_up");
			}				
		}			

		if (GetBool("holdingDown"))
			SetState("look_down");

		if (GetBool("walking"))
			SetState("walk");

		if (!GetBool("isGrounded") && !GetBool("hasParent"))
			SetState("jump");

		if (GetBool("isCastingDebug"))
		{
			if (GetBool("holdingUp"))
				SetState("debug_up");
			else if (GetBool("holdingDown"))
				SetState("debug_down");
			else
				SetState("debug");
		}
	}
	else if (currentState->name == "jump")
	{
		if (GetBool("holdingUp"))
		{
			if (GetBool("onLadder"))
			{
				SetState("ladder_idle");
				return;
			}
		}

		if (GetBool("isGrounded") || GetBool("hasParent"))
			SetState("idle");

		if (GetBool("isCastingDebug"))
		{
			if (GetBool("holdingUp"))
				SetState("debug_air_up");
			else if (GetBool("holdingDown"))
				SetState("debug_air_down");
			else
				SetState("debug_air");
		}			
	}
	else if (IsStateDebugSpell())
	{
		StateKanekoDebugSpell();
	}
	else if (currentState->name == "PUSH")
	{
		if (animationTimer.HasElapsed())
		{
			SetBool("isCastingSpell", false);

			if (GetBool("isGrounded"))
				SetState("idle");
			else
				SetState("jump");
		}
	}
}

bool Animator::IsStateDebugSpell()
{
	return (currentState->name == "debug" || currentState->name == "debug_up" ||
		currentState->name == "debug_down" || currentState->name == "debug_air" ||
		currentState->name == "debug_air_up" || currentState->name == "debug_air_down");
}

void Animator::StateKanekoDebugSpell()
{
	if (animationTimer.HasElapsed())
	{
		SetBool("isCastingDebug", false);

		if (GetBool("isGrounded"))
			SetState("idle");
		else
			SetState("jump");
	}
}

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
	return mapParamsBool[param];
}

void Animator::SetBool(const char* param, bool value)
{
	mapParamsBool[param] = value;
}

void Animator::SetFloat(const char* param, float value)
{
	mapParamsFloat[param] = value;
}

void Animator::SetInt(const char* param, int value)
{
	mapParamsInt[param] = value;
}