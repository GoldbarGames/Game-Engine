#include "Player.h"
#include "Game.h"
#include "debug_state.h"
#include <string>

Player::Player()
{

}

Player::~Player()
{

}

void Player::Update(Game& game)
{
	animator->SetBool("walking", false);

	//Set texture based on current keystate
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);

	if (currentKeyStates[SDL_SCANCODE_UP] || currentKeyStates[SDL_SCANCODE_W])
	{

	}

	if (currentKeyStates[SDL_SCANCODE_DOWN] || currentKeyStates[SDL_SCANCODE_S])
	{

	}

	if (currentKeyStates[SDL_SCANCODE_LEFT] || currentKeyStates[SDL_SCANCODE_A])
	{
		animator->SetBool("walking", true);
		velocity.x -= horizontalSpeed;
		
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		animator->SetBool("walking", true);
		velocity.x += horizontalSpeed;
	}
	else
	{
		//TODO: Add friction
		velocity.x = 0;
	}

	UpdatePhysics(game);

	if (animator != nullptr)
		animator->Update(this);
}

void Player::UpdatePhysics(Game& game)
{
	const float GRAVITY = 0.001f; //TODO: Better way of handling gravity

	if (velocity.y < 1)
		velocity.y += GRAVITY;
	
	if (game.pressedJumpButton && jumpsRemaining > 0)
	{
		velocity.y = -0.4f;
	}

	CheckCollisions(game);

}

void Player::CheckCollisions(Game& game)
{
	bool horizontalCollision = false;
	bool verticalCollision = false;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetBounds();

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = myBounds.x + (velocity.x * game.dt);

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = myBounds.y + (velocity.y * game.dt);

	// this needs to be here so that it does not check for horizontal collision when moving vertically
	if (velocity.x > 0)
		newBoundsVertical.x -= 1; 
	else if (velocity.x < 0)
		newBoundsVertical.x += 1;

	// this needs to be here so that it does not check for vertical collision when moving horizontally
	if (velocity.y > 0)
		newBoundsHorizontal.y -= 1;
	else if (velocity.y < 0)
		newBoundsHorizontal.y += 1;


	animator->SetBool("isGrounded", true);

	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		if (horizontalCollision && verticalCollision)
			break;

		const SDL_Rect * theirBounds = game.entities[i]->GetBounds();

		if (game.entities[i] != this && game.entities[i]->impassable)
		{
			if (SDL_HasIntersection(&newBoundsHorizontal, theirBounds))
			{
				horizontalCollision = true;
				velocity.x = 0;
			}
			
			if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
			{
				verticalCollision = true;
				
				// if colliding with ground, set velocity.y to zero
				if (velocity.y > 0)
				{
					animator->SetBool("isGrounded", true);
					jumpsRemaining = 2;
				}	

				velocity.y = 0;
			}
		}
	}

	if (!horizontalCollision)
	{
		position.x += (velocity.x * game.dt);
	}

	if (!verticalCollision)
	{
		if (game.pressedJumpButton && jumpsRemaining > 0)
		{
			jumpsRemaining--;
			game.jumpsRemainingText->SetText("Jumps Remaining: " + std::to_string(jumpsRemaining));
		}

		position.y += (velocity.y * game.dt);
	}
}

void Player::ResetPosition()
{
	position = startPosition;
}

void Player::Render(SDL_Renderer * renderer, Vector2 cameraOffset)
{
	if (currentSprite != nullptr)
	{
		if (animator != nullptr)
			currentSprite->Render(position - cameraOffset, animator->speed, renderer);
		else
			currentSprite->Render(position - cameraOffset, 0, renderer);

		if (GetModeDebug())
		{
			if (impassable)
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			else
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

			SDL_RenderDrawRect(renderer, currentSprite->GetRect());
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}
	}
}