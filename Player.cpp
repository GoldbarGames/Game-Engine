#include "Player.h"
#include "Game.h"
#include "debug_state.h"
#include <string>

Player::Player()
{
	CreateCollider(0, 0, 0.75f, 0.9f);
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
		velocity.y = -0.6f;
	}

	CheckCollisions(game);
}

void Player::CheckCollisions(Game& game)
{
	// method 1
	//pivot = game.spriteManager.GetPivotPoint(currentSprite->filename);

	//method 2 (we need to set the pivot member variable for the Render function!)
	pivot = currentSprite->pivot;
	CalculateCollider(game.camera);

	bool horizontalCollision = false;
	bool verticalCollision = false;

	// Get bounds assuming the move is valid
	SDL_Rect myBounds = *GetColliderBounds();

	SDL_Rect newBoundsHorizontal = myBounds;
	newBoundsHorizontal.x = myBounds.x + (velocity.x * game.dt);

	SDL_Rect newBoundsVertical = myBounds;
	newBoundsVertical.y = myBounds.y + (velocity.y * game.dt);

	// this needs to be here so that it does not check for horizontal collision when moving vertically
	if (velocity.x > 0)
	newBoundsVertical.x -= 1;
	else if (velocity.x < 0)
	{
		newBoundsVertical.x += 1;
		newBoundsHorizontal.x -= 1;
	}
	else
	newBoundsVertical.x -= 1;

	// this needs to be here so that it does not check for vertical collision when moving horizontally
	if (velocity.y > 0)
	newBoundsHorizontal.y -= 1;
	else if (velocity.y < 0)
		newBoundsHorizontal.y += 1;

	animator->SetBool("isGrounded", false);

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
		// if the anim that we are going to is the same
		// as the anim we previously left,
		// then dont offset based on the pivot

		/*
		if (animator->currentState == "idle" || animator->currentState == "walk")
		{
			pivotDifference = Vector2(0, 0);
		}
		else if (animator->currentState == "jump")
		{
			pivotDifference = Vector2(-4, 0);
		}
		*/

		Vector2 offset = cameraOffset + pivotDifference;

		float pdX = (pivot.x * SCALE) - (previousPivot.x * SCALE);
		float pdY = (pivot.y * SCALE) - (previousPivot.y * SCALE);

		pivotDifference = Vector2(pdX, pdY);
		offset = cameraOffset + pivotDifference;

		if (pivotDifference.x != 0)
			int test = 0;

		/*
		if (animator != nullptr && animator->beforePreviousState != animator->currentState)
		{
			float pdX = (pivot.x * SCALE) - (previousPivot.x * SCALE);
			float pdY = (pivot.y * SCALE) - (previousPivot.y * SCALE);

			pivotDifference = Vector2(pdX * SCALE, pdY * SCALE);
			offset = cameraOffset + pivotDifference;

			if (pivotDifference.x != 0)
				int test = 0;
		}*/
		
		if (animator != nullptr)
			currentSprite->Render(position - offset, animator->speed, renderer);
		else
			currentSprite->Render(position - offset, 0, renderer);

		if (GetModeDebug())
		{
			if (impassable)
				SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			else
				SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);

			SDL_RenderDrawRect(renderer, currentSprite->GetRect());
			
			SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			CalculateCollider(cameraOffset); //TODO: better way than calculating this twice?
			
			SDL_RenderDrawRect(renderer, collisionBounds);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		}
	}

	previousPivot = pivot;
}

void Player::CalculateCollider(Vector2 cameraOffset)
{
	// scale the bounds of the sprite by a number
	collisionBounds->w = startSpriteSize.x * colliderWidth;
	collisionBounds->h = startSpriteSize.y * colliderHeight;

	// set the collision bounds position to where the player actually is
	collisionBounds->x = position.x + collider->x - cameraOffset.x;
	collisionBounds->y = position.y + collider->y - cameraOffset.y;

	/*
	// get the distance to the center of the sprite
	float halfWidth = (currentSprite->GetRect()->w / (2.0f));
	float halfHeight = (currentSprite->GetRect()->h / (2.0f));

	// get the position of the center of the sprite (in world space)
	float positionCenterX = collisionBounds->x + halfWidth;
	float positionCenterY = collisionBounds->y + halfHeight;

	// get the distance to the pivot point from the center of the sprite
	pivotDistance = Vector2(16 - (halfWidth / SCALE), 24 - (halfHeight / SCALE));

	// set the position such that the center is at the pivot point
	collisionBounds->x = positionCenterX + (pivotDistance.x * SCALE * colliderWidth) - halfWidth;
	collisionBounds->y = positionCenterY + (pivotDistance.y * SCALE * colliderHeight) - halfHeight;

	// set the position such that the center is at the center
	collisionBounds->x += (currentSprite->GetRect()->w - collisionBounds->w) / 2.0f;
	collisionBounds->y += (currentSprite->GetRect()->h - collisionBounds->h) / 2.0f;
	*/
}