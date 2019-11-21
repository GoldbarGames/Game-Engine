#include "Player.h"
#include "Game.h"
#include "debug_state.h"
#include <string>
#include "Physics.h"
#include "SpellPush.h"
#include "SpellPop.h"

Player::Player(Vector2 pos) : PhysicsEntity(pos)
{
	etype = "player";
	standAboveGround = true;
	CreateCollider(27, 46, 0, 0, 20.25f, 41.40f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 99;

	trigger = false;

	// Initialize the spells here
	spells.clear();
	spells.push_back(new SpellPush("PUSH"));
	spells.push_back(new SpellPop("POP"));

	//TODO: Pause all timers when game is paused
	missileTimer.Start(1);
	doorTimer.Start(1);	
	spellTimer.Start(1);
}

Player::~Player()
{

}

void Player::Render(Renderer * renderer, Vector2 cameraOffset)
{
	PhysicsEntity::Render(renderer, cameraOffset);
}

void Player::Update(Game& game)
{	
	static unsigned int count = 0;

	if (game.watchingCutscene)
	{
		//TODO: Get input for handling the textbox
		animator->SetBool("holdingUp",  false);

		if (!game.cutscene->isReadingNextLine)
		{
			const Uint8* input = SDL_GetKeyboardState(NULL);

			if (input[SDL_SCANCODE_DOWN])
			{
				game.cutscene->ReadNextLine();
			}
		}
		else
		{
			//TODO: If we press the button before the line has finished displaying,
			// then instantly show all the text (maybe a different button)
		}
	}
	else
	{
		if (!animator->GetBool("isCastingSpell"))
			UpdateNormally(game);
	}

	std::cout << "--" << count << "--" << std::endl;
	std::cout << GetPosition() << std::endl;

	count++;
	
	UpdateAnimator();
}

void Player::UpdateAnimator()
{
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
			game.cutscene->PlayCutscene(currentNPC->cutsceneLabel);
		}
		else if (currentGoal != nullptr)
		{
			if (currentGoal->isOpen)
			{
				game.goToNextLevel = true;
				game.nextLevel = currentGoal->nextLevelName;
				return;
			}			
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
			if (currentLadder->GetAnimator()->currentState->name != "top")
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
	else if (game.pressedSpellButton && spellTimer.HasElapsed())
	{
		if (!animator->GetBool("isCastingSpell"))
			spells[spellIndex]->Cast(game);
		// TODO: Reset the spell timer somehow
	}

	//TODO: What should happen if multiple buttons are pressed at the same time?

	if (game.pressedLeftTrigger)
	{
		spellIndex--;
		if (spellIndex < 0)
			spellIndex = 0;
	}
	else if (game.pressedRightTrigger)
	{
		spellIndex++;
		if (spellIndex > spells.size() - 1)
			spellIndex = spells.size() - 1;
	}

	// If on the ladder, only move up or down
	if (animator->GetBool("onLadder"))
	{
		GetLadderInput(input);

		CheckJumpButton(input);

		CheckCollisions(game);
	}
	else
	{
		// Don't move if we are casting debug, or looking up/down
		if (!animator->GetBool("holdingUp") && !animator->GetBool("holdingDown"))
		{
			GetMoveInput(input);
		}
		else if (animator->GetBool("isGrounded"))
		{
			velocity.x = 0;
		}

		// Update Physics
		if (velocity.y < 1)
			velocity.y += Physics::GRAVITY * game.dt;

		CheckJumpButton(input);

		CheckCollisions(game);
	}
}

void Player::CastSpellDebug(Game &game, const Uint8* input)
{
	if (game.currentEther <= 0)
		return;

	Vector2 missilePosition = this->position;
	missilePosition.x += (this->currentSprite->GetRect()->w / 2);
	missilePosition.y += (this->currentSprite->GetRect()->h / 2);

	const float missileSpeed = maxHorizontalSpeed;
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

	Missile* missile = game.SpawnMissile(missilePosition, missileVelocity, angle);
	if (missile != nullptr)
	{
		game.currentEther--;
		game.etherText->SetText("Ether: " + std::to_string(game.currentEther));

		missile->etype = "debug_missile";
		game.soundManager->PlaySound("shoot", 1);
		animator->SetBool("isCastingDebug", true);
		missileTimer.Start(750);
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

void Player::CheckJumpButton(const Uint8* input)
{
	hadPressedJump = pressingJumpButton;
	pressingJumpButton = input[SDL_SCANCODE_X];
	//if (pressingJumpButton)
	//	std::cout << "!!!!" << std::endl;

	canJump = ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0);

	/*
	if ((!hadPressedJump && pressingJumpButton) && jumpsRemaining > 0)
	{
		if (animator->GetBool("holdingUp") || animator->GetBool("holdingDown"))
		{
			canJump = false;
		}
		else
		{
			canJump = true;
		}
	}*/
}


void Player::ResetPosition()
{
	position = startPosition;
}

void Player::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Property*>& properties)
{
	Entity::GetProperties(renderer, font, properties);

	properties.emplace_back(new Property(new Text(renderer, font, "Start Pos X: " + std::to_string((int)startPosition.x))));
	properties.emplace_back(new Property(new Text(renderer, font, "Start Pos Y: " + std::to_string((int)startPosition.y))));
}

void Player::SetProperty(std::string prop, std::string newValue)
{
	// 1. Split the string into two (key and value)
	std::string key = "";

	int index = 0;
	while (prop[index] != ':')
	{
		key += prop[index];
		index++;
	}

	// 2. Based on the key, change its value
	if (key == "Start Pos X")
	{
		if (newValue != "")
			startPosition.x = std::stof(newValue);
	}
	else if (key == "Start Pos Y")
	{
		if (newValue != "")
			startPosition.y = std::stof(newValue);
	}
}

void Player::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << startPosition.x << " " << startPosition.y << " " << drawOrder <<
		" " << layer << " " << impassable << std::endl;
}