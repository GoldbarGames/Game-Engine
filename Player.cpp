#include "Player.h"
#include "Game.h"

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
		velocity.x -= horizontalSpeed * game.dt;
		
	}
	else if (currentKeyStates[SDL_SCANCODE_RIGHT] || currentKeyStates[SDL_SCANCODE_D])
	{
		animator->SetBool("walking", true);
		velocity.x += horizontalSpeed * game.dt;
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
	const float GRAVITY = 0.002f; //TODO: Better way of handling gravity
	velocity.y += GRAVITY;

	if (game.pressedJumpButton)
	{
		velocity.y = -1.2f;
	}

	bool collideX = false;
	bool collideY = false;

	CheckCollisions(game, collideX, collideY);

	if (!collideX)
	{
		position.x += (velocity.x * game.dt);
	}

	if (!collideY)
	{
		position.y += (velocity.y * game.dt);
	}

}

void Player::CheckCollisions(Game& game, bool& collideX, bool& collideY)
{
	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetBounds();

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = myBounds.x + velocity.x;

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = myBounds.y + velocity.y;

	// Negate space that checks collision in the wrong axis
    //TODO: Can we come up with a better solution?
    //TODO: Maybe use 2 for loops instead? Or 2 hitboxes?
	newBoundsHorizontal.y += velocity.y;
	newBoundsHorizontal.h -= velocity.y * 2;

	newBoundsVertical.x += velocity.x;
	newBoundsVertical.w -= velocity.x * 2;

	for (int i = 0; i < game.entities.size(); i++)
	{
		if (collideX && collideY)
			break;

		const SDL_Rect * theirBounds = game.entities[i]->GetBounds();

		if (game.entities[i] != this && game.entities[i]->impassable)
		{
			if (SDL_HasIntersection(&newBoundsHorizontal, theirBounds))
			{
				collideX = true;
			}

			if (SDL_HasIntersection(&newBoundsVertical, theirBounds))
			{
				collideY = true;

				// if colliding with ground, set velocity.y to zero
				if (theirBounds->y >= myBounds.y + myBounds.h - 1)
					velocity.y = 0;
			}
		}
	}
}

void Player::ResetPosition()
{
	position = startPosition;
}