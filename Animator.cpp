#include "Animator.h"
#include "Entity.h"
#include <utility>

Animator::Animator(std::vector<Sprite*> sprites)
{

}

// PRE-CONDITION: Initialize all anim parameters
Animator::Animator(std::string initialState)
{
	// Set the initial state
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

	if (currentState == "walk")
	{
		if (!GetBool("walking"))
			SetState("idle");

		if (!GetBool("isGrounded"))
			SetState("jump");
	}
	else if (currentState == "blink")
	{
		if (GetBool("walking"))
			SetState("walk");

		if (!GetBool("isGrounded"))
			SetState("jump");
	}
	else if (currentState == "idle")
	{
		if (GetBool("walking"))
			SetState("walk");
		
		if (!GetBool("isGrounded"))
			SetState("jump");
	}
	else if (currentState == "jump")
	{
		if (GetBool("isGrounded"))
			SetState("idle");
	}

	// Then, carry out whatever the current state is
	DoState(entity);
}

void Animator::SetState(std::string state)
{
	beforePreviousState = previousState;
	previousState = currentState;	
	currentState = state;
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