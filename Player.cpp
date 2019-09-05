#include "Player.h"
#include "Game.h"
#include "debug_state.h"
#include <string>

Player::Player(Vector2 pos) : PhysicsEntity(pos)
{
	etype = "player";
	CreateCollider(27, 46, 0, 0, 0.75f, 0.9f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 99;

	//TODO: Pause all timers when game is paused
	missileTimer.Start(1);
	doorTimer.Start(1);	
}

Player::~Player()
{

}

void Player::Update(Game& game)
{	
	if (game.watchingCutscene)
	{
		//TODO: Get input for handling the textbox
		animator->SetBool("holdingUp",  false);
	}
	else
	{
		UpdateNormally(game);
	}
	
	if (animator != nullptr)
		animator->Update(this);
}

void Player::UpdateNormally(Game& game)
{
	//Set texture based on current keystate
	const Uint8* input = SDL_GetKeyboardState(NULL);

	bool wasHoldingUp = animator->GetBool("holdingUp");

	animator->SetBool("walking", false);
	animator->SetBool("holdingUp", input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W]);
	animator->SetBool("holdingDown", input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S]);

	// if we are holding up now and were not before...
	if (animator->GetBool("holdingUp") && !wasHoldingUp)
	{
		// if we are in front of a door, ladder, or NPC...

		if (currentNPC != nullptr)
		{
			//TODO: Display the textbox and stuff
			game.watchingCutscene = true;
		}
		else if (currentDoor != nullptr && doorTimer.HasElapsed())
		{
			//TODO: Make this look better later
			SetPosition(currentDoor->GetDestination() + CalcScaledPivot());
			doorTimer.Start(500);
		}
		else if (currentLadder != nullptr)
		{
			// TODO: What if there is a door and a ladder at the same spot?
			if (currentLadder->GetAnimator()->currentState != "top")
			{
				velocity.y = 0;
				animator->SetBool("onLadder", true);
			}
		}
	}

	//TODO: Should we limit the number that can be spawned?
	//TODO: Add a time limit between shots
	if (game.pressedDebugButton && missileTimer.HasElapsed())
	{
		CastSpellDebug(game, input);
	}


	// If on the ladder, only move up or down
	if (animator->GetBool("onLadder"))
	{
		GetLadderInput(input);

		CheckJumpButton(game);

		CheckCollisions(game);
	}
	else
	{
		// Don't move if we are casting debug, or looking up/down
		if (!animator->GetBool("isCastingDebug") && !animator->GetBool("holdingUp")
			&& !animator->GetBool("holdingDown"))
		{
			GetMoveInput(input);
		}

		UpdatePhysics(game);
	}
}

void Player::CastSpellDebug(Game &game, const Uint8* input)
{
	Vector2 missilePosition = this->position;
	missilePosition.x += (this->currentSprite->GetRect()->w / 2);
	missilePosition.y += (this->currentSprite->GetRect()->h / 2);

	const float missileSpeed = 0.1f;
	Vector2 missileVelocity = Vector2(0, 0);

	float angle = 0;

	if (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W])
	{
		missileVelocity.y = -missileSpeed;
		angle = 270;
	}
	else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
	{
		missileVelocity.y = missileSpeed;
		angle = 90;
	}
	else if (flip == SDL_FLIP_NONE)
	{
		missileVelocity.x = missileSpeed;
	}
	else if (flip == SDL_FLIP_HORIZONTAL)
	{
		missileVelocity.x = -missileSpeed;
		angle = 180;
	}

	if (game.SpawnMissile(missilePosition, missileVelocity, angle))
	{
		animator->SetBool("isCastingDebug", true);
		missileTimer.Start(1000);
	}
}

void Player::GetLadderInput(const Uint8* input)
{
	animator->SetBool("climbing", false);

	if (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W])
	{
		animator->SetBool("climbing", true);
		velocity.y -= horizontalSpeed;
	}
	else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
	{
		animator->SetBool("climbing", true);
		velocity.y += horizontalSpeed;
	}
	else
	{
		velocity.y = 0;
	}

	if (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A])
	{
		animator->SetBool("climbing", true);
		velocity.x -= horizontalSpeed;
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D])
	{
		animator->SetBool("climbing", true);
		velocity.x += horizontalSpeed;
		flip = SDL_FLIP_NONE;
	}
	else
	{
		velocity.x = 0;
	}

	if (velocity.y > maxHorizontalSpeed)
		velocity.y = maxHorizontalSpeed;
	else if (velocity.y < -maxHorizontalSpeed)
		velocity.y = -maxHorizontalSpeed;

	if (velocity.x > maxHorizontalSpeed)
		velocity.x = maxHorizontalSpeed;
	else if (velocity.x < -maxHorizontalSpeed)
		velocity.x = -maxHorizontalSpeed;
}

void Player::GetMoveInput(const Uint8* input)
{
	if (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A])
	{
		animator->SetBool("walking", true);
		velocity.x -= horizontalSpeed;
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D])
	{
		animator->SetBool("walking", true);
		velocity.x += horizontalSpeed;
		flip = SDL_FLIP_NONE;
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

void Player::CheckJumpButton(Game& game)
{
	if (game.pressedJumpButton && jumpsRemaining > 0)
	{
		if (animator->GetBool("holdingUp") || animator->GetBool("holdingDown"))
		{
			game.pressedJumpButton = false;
		}
		else
		{
			velocity.y = -0.6f;
		}
	}
}

void Player::UpdatePhysics(Game& game)
{
	const float GRAVITY = 0.002f; //TODO: Better way of handling gravity

	if (velocity.y < 1)
		velocity.y += GRAVITY;
	
	CheckJumpButton(game);

	CheckCollisions(game);
}



void Player::ResetPosition()
{
	position = startPosition;
}




