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

	// Then, carry out whatever the current state is
	DoState(entity);
}

void Animator::CheckStateKaneko()
{
	if (currentState == "walk")
	{
		if (!GetBool("walking"))
			SetState("idle");

		if (!GetBool("isGrounded"))
			SetState("jump");

		if (GetBool("isCastingDebug"))
			SetState("debug");
	}
	else if (currentState == "blink")
	{
		if (GetBool("walking"))
			SetState("walk");

		if (!GetBool("isGrounded"))
			SetState("jump");

		if (GetBool("isCastingDebug"))
			SetState("debug");
	}
	else if (currentState == "idle")
	{
		if (GetBool("walking"))
			SetState("walk");

		if (!GetBool("isGrounded"))
			SetState("jump");

		if (GetBool("isCastingDebug"))
			SetState("debug");
	}
	else if (currentState == "jump")
	{
		if (GetBool("isGrounded"))
			SetState("idle");

		if (GetBool("isCastingDebug"))
			SetState("debug_air");
	}
	else if (currentState == "debug")
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
	else if (currentState == "debug_air")
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
}

void Animator::SetState(std::string state)
{
	beforePreviousState = previousState;
	previousState = currentState;	
	currentState = state;

	if (currentState == "debug" || currentState == "debug_air")
		speed = 50;
	else
		speed = 100;

	// set duration of the animation based on the playback speed and number of frames
	animationTimer.Start(speed * mapStateToSprite[currentState]->endFrame, mapStateToSprite[currentState]->shouldLoop);
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