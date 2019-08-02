#include "Player.h"
#include "Game.h"
#include "debug_state.h"
#include <string>

Player::Player()
{
	CreateCollider(27, 46, 0, 0, 0.75f, 0.9f);
	missileTimer.Start(1);
}

Player::~Player()
{

}

void Player::Update(Game& game)
{
	animator->SetBool("walking", false);

	//TODO: Should we limit the number that can be spawned?
	//TODO: Add a time limit between shots
	if (game.pressedDebugButton && missileTimer.HasElapsed())
	{
		Vector2 missilePosition = this->position;
		missilePosition.x += (this->currentSprite->GetRect()->w / 2);
		missilePosition.y += (this->currentSprite->GetRect()->h / 2);

		if (game.SpawnMissile(missilePosition))
		{
			animator->SetBool("isCastingDebug", true);
			missileTimer.Start(1000);
		}			
	}	

	if (!animator->GetBool("isCastingDebug"))
	{
		GetMoveInput();
	}

	UpdatePhysics(game);

	if (animator != nullptr)
		animator->Update(this);
}

void Player::GetMoveInput()
{
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

	if (velocity.x > maxHorizontalSpeed)
		velocity.x = maxHorizontalSpeed;
	else if (velocity.x < -maxHorizontalSpeed)
		velocity.x = -maxHorizontalSpeed;
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

//TODO: Check that the entity we are colliding with is not to be destroyed on the next frame?
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
	{
		newBoundsVertical.x -= 1;
	}	
	else if (velocity.x < 0)
	{
		newBoundsVertical.x += 1;
		newBoundsHorizontal.x -= 1;
	}
	else
	{
		newBoundsVertical.x -= 1;
	}
	
	// this needs to be here so that it does not check for vertical collision when moving horizontally
	if (velocity.y > 0)
	{
		newBoundsHorizontal.y -= 1;
	}
	else if (velocity.y < 0)
	{
		newBoundsHorizontal.y += 1;
	}
		
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
		float collisionCenterX = (collisionBounds->x + (collisionBounds->w / 2));
		float collisionCenterY = (collisionBounds->y + (collisionBounds->h / 2));
		Vector2 collisionCenter = Vector2(collisionCenterX, collisionCenterY);
		Vector2 scaledPivot = Vector2(currentSprite->pivot.x * SCALE, currentSprite->pivot.y * SCALE);
		Vector2 pivotOffset = collisionCenter - scaledPivot;

		Vector2 offset = pivotOffset;

		if (GetModeEdit())
			offset -= cameraOffset;
		
		if (animator != nullptr)
			currentSprite->Render(offset, animator->speed, animator->animationTimer.GetTicks(), renderer);
		else
			currentSprite->Render(offset, 0, -1, renderer);

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
	// set the collision bounds position to where the player actually is
	collisionBounds->x = position.x + collider->x - cameraOffset.x;
	collisionBounds->y = position.y + collider->y - cameraOffset.y;

	// scale the bounds of the sprite by a number to set the collider's width and height
	collisionBounds->w = startSpriteSize.x * colliderWidth;
	collisionBounds->h = startSpriteSize.y * colliderHeight;
}