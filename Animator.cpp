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
	// If conditions met, set current state to next state
	// Else, stay in current state

	if (currentState == "walk")
	{
		if (!GetBool("walking"))
			SetState("blink");
	}
	else if (currentState == "blink")
	{
		if (GetBool("walking"))
			SetState("walk");
	}

	// Then, carry out whatever the current state is
	DoState(entity);
}

void Animator::SetState(std::string state)
{
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