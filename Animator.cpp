#include "Animator.h"
#include "Entity.h"
#include <utility>

Animator::Animator(std::vector<Sprite*> sprites)
{

}

// PRE-CONDITION: Initialize all anim parameters
Animator::Animator(std::string animType, std::string initialState)
{
	// Set the initial state
	animatorType = animType;
	currentState = initialState;
	previousState = currentState;
	beforePreviousState = previousState;	
}

Animator::Animator(SpriteManager * spriteManager, SDL_Renderer * renderer)
{

}

Animator::~Animator()
{

}

void Animator::MapStateToSprite(std::string state, Sprite* sprite)
{
	mapStateToSprite[state] = sprite;
}

void Animator::OnEnter(std::string state)
{

}

void Animator::DoState(Entity* entity)
{
	entity->SetSprite(mapStateToSprite[currentState]);
}

void Animator::OnExit(std::string state)
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
		if (currentState == "moving")
		{
			if (GetBool("destroyed"))
				SetState("destroyed");
		}
	}
	else if (animatorType == "door")
	{
		if (currentState == "closed")
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
	if (currentState == "look_up")
	{
		if (!GetBool("holdingUp"))
		{
			SetState("idle");
		}
		else if (GetBool("isCastingDebug"))
		{
			SetState("debug_air_up");
		}
	}
	else if (currentState == "look_down")
	{
		if (!GetBool("holdingDown"))
		{
			SetState("idle");
		}
		else if (GetBool("isCastingDebug"))
		{
			SetState("debug_air_down");
		}
	}

	if (currentState == "walk")
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
	else if (currentState == "blink")
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
	else if (currentState == "ladder_idle")
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
	else if (currentState == "ladder_climbing")
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
	else if (currentState == "idle")
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
	else if (currentState == "jump")
	{
		if (GetBool("holdingUp"))
		{
			if (GetBool("onLadder"))
			{
				SetState("ladder_idle");
				return;
			}
		}

		if (GetBool("isGrounded"))
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
}

bool Animator::IsStateDebugSpell()
{
	return (currentState == "debug" || currentState == "debug_up" || currentState == "debug_down" ||
		currentState == "debug_air" || currentState == "debug_air_up" || currentState == "debug_air_down");
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
	animationTimer.Start(speed * mapStateToSprite[currentState]->endFrame, mapStateToSprite[currentState]->shouldLoop);
}

Sprite* Animator::GetCurrentSprite()
{
	return mapStateToSprite[currentState];
}

void Animator::SetState(std::string state)
{
	beforePreviousState = previousState;
	previousState = currentState;	
	currentState = state;

	if (IsStateDebugSpell())
		speed = 50;
	else
		speed = 100;

	StartTimer();
}

bool Animator::GetBool(std::string param)
{
	//TODO: Check if it exists first
	return mapParamsBool[param];
}

void Animator::SetBool(std::string param, bool value)
{
	mapParamsBool[param] = value;
}

void Animator::SetFloat(std::string param, float value)
{
	mapParamsFloat[param] = value;
}

void Animator::SetInt(std::string param, int value)
{
	mapParamsInt[param] = value;
}