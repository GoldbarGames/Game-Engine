#include "Player.h"
#include "Game.h"
#include <string>
#include "PhysicsInfo.h"
#include "SpellPush.h"
#include "SpellPop.h"
#include "Animator.h"
#include "Physics.h"

Player::Player(Vector2 pos) : Entity(pos)
{
	etype = "player";
	
	CreateCollider(0, 0, 20.25f, 41.40f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 99;
	
	trigger = false;

	physics = new PhysicsInfo(this);
	physics->standAboveGround = true;

	// Initialize the spells here
	spell = Spell("PUSH");

	//TODO: Pause all timers when game is paused
	missileTimer.Start(1);
	doorTimer.Start(1);	
	spellTimer.Start(1);

	//rotation = glm::vec3(90.0f, 0.0f, 0.0f);
}

Player::~Player()
{

}

void Player::Render(Renderer * renderer)
{
	Entity::Render(renderer);
}

void Player::Update(Game& game)
{	
	static unsigned int count = 0;

	//TODO: Change this so that we collide with an object instead of hard-coding a number
	// Also, maybe draw an outline of the death barrier so the player can see where this is
	if (position.y > 500)
	{
		game.state = GameState::RESET_LEVEL;
		return;
	}		

	if (game.cutscene->watchingCutscene)
	{
		//TODO: Get input for handling the textbox
		animator->SetBool("holdingUp",  false);
	}
	else
	{
		if (!castingSpell)
			UpdateNormally(game);
	}

	//std::cout << "--" << count << "--" << std::endl;
	//std::cout << GetPosition() << std::endl;

	//count++;
	
	UpdateAnimator();
}

void Player::UpdateAnimator()
{
	if (castingDebug && animator->animationTimer.HasElapsed())
	{
		animator->SetBool("isCastingDebug", false);
		castingDebug = false;
	}

	if (castingSpell && animator->animationTimer.HasElapsed())
	{
		animator->SetBool("isCastingSpell", false);
		castingSpell = false;
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

	if (currentLadder != nullptr && !physics->hadPressedJump && physics->pressingJumpButton)
	{
		GetAnimator()->SetBool("onLadder", false);
		currentLadder = nullptr;
	}

	// if we are holding up now and were not before...
	if (animator->GetBool("holdingUp"))
	{
		// if we are in front of a door, ladder, or NPC...

		// for when up is pressed
		if (!wasHoldingUp)
		{
			if (currentNPC != nullptr)
			{
				game.cutscene->PlayCutscene(currentNPC->cutsceneLabel.c_str());
			}
			else if (currentGoal != nullptr)
			{
				if (currentGoal->isOpen)
				{
					game.state = GameState::LOAD_NEXT_LEVEL;
					game.nextLevel = currentGoal->nextLevelName;
					return;
				}
			}
			else if (currentDoor != nullptr && doorTimer.HasElapsed())
			{
				//TODO: Make this look better later
				// Should this play a cutscene here?
				SetPosition(currentDoor->GetDestination() + physics->CalcScaledPivot());
				doorTimer.Start(500);
			}
			else if (currentLadder != nullptr)
			{
				// TODO: Maybe make this a function or refactor to something better?
				// TODO: What if there is a door and a ladder at the same spot?
				if (currentLadder->GetAnimator()->currentState->name != "top")
				{
					//std::cout << "ladder 1" << std::endl;
					physics->velocity.y = 0;
					physics->velocity.x = 0;
					animator->SetBool("onLadder", true);
				}
			}
		}

		// for when up is being held
		if (currentLadder != nullptr)
		{
			//std::cout << "ladder NOT null" << std::endl;
			// TODO: What if there is a door and a ladder at the same spot?
			if (currentLadder->GetAnimator()->currentState->name != "top")
			{
				//std::cout << "ladder 2" << std::endl;
				physics->velocity.y = 0;
				physics->velocity.x = 0;
				animator->SetBool("onLadder", true);
			}
		}		
	}

	//TODO: Should we limit the number that can be spawned?
	//TODO: Add a time limit between shots
	if (game.pressedDebugButton && missileTimer.HasElapsed() && !castingDebug)
	{
		CastSpellDebug(game, input);
	}
	else if (game.pressedSpellButton && spellTimer.HasElapsed() && !castingSpell)
	{
		spell.Cast(game);
		// TODO: Reset the spell timer somehow
	}

	//TODO: What should happen if multiple buttons are pressed at the same time?
	if (game.pressedLeftTrigger)
	{
		spell.activeSpell--;
		if (spell.activeSpell < 0)
			spell.activeSpell = 0;
	}
	else if (game.pressedRightTrigger)
	{
		spell.activeSpell++;
		//if (spell.activeSpell > spells.size() - 1)
		//	spell.activeSpell = spells.size() - 1;
	}

	// If on the ladder, only move up or down
	if (animator->GetBool("onLadder"))
	{
		GetLadderInput(input);

		CheckJumpButton(input);

		physics->CheckCollisions(game);
	}
	else
	{
		// Don't move if we are casting debug, or looking up/down
		if (!animator->GetBool("holdingUp") && !animator->GetBool("holdingDown"))
		{
			GetMoveInput(input);
		}
		else if (physics->isGrounded)
		{
			physics->velocity.x = 0;
		}

		// Update Physics
		if (physics->velocity.y < 1)
			physics->velocity.y += Physics::GRAVITY * game.dt;

		CheckJumpButton(input);

		physics->CheckCollisions(game);
	}
}

void Player::CastSpellDebug(Game &game, const Uint8* input)
{
	if (game.currentEther <= 0)
		return;

	castingDebug = true;

	Vector2 missilePosition = this->position;
	//missilePosition.x += (this->currentSprite->GetRect()->w / 2);
	//missilePosition.y += (this->currentSprite->GetRect()->h / 2);

	const float missileSpeed = physics->maxHorizontalSpeed;
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
		game.soundManager->PlaySound("se/shoot.wav", 1);
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
		physics->velocity.y -= physics->horizontalSpeed * 0.5f;
	}
	else if (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S])
	{
		animator->SetBool("climbing", true);
		physics->velocity.y += physics->horizontalSpeed * 0.5f;
	}
	else
	{
		physics->velocity.y = 0;
	}

	if (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A])
	{
		animator->SetBool("climbing", true);
		physics->velocity.x -= physics->horizontalSpeed * 0.15f;
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D])
	{
		animator->SetBool("climbing", true);
		physics->velocity.x += physics->horizontalSpeed * 0.15f;
		flip = SDL_FLIP_NONE;
	}
	else
	{
		physics->velocity.x = 0;
	}

	if (physics->velocity.y > physics->maxHorizontalSpeed)
		physics->velocity.y = physics->maxHorizontalSpeed;
	else if (physics->velocity.y < -physics->maxHorizontalSpeed)
		physics->velocity.y = -physics->maxHorizontalSpeed;

	if (physics->velocity.x > physics->maxHorizontalSpeed)
		physics->velocity.x = physics->maxHorizontalSpeed;
	else if (physics->velocity.x < -physics->maxHorizontalSpeed)
		physics->velocity.x = -physics->maxHorizontalSpeed;
}

void Player::GetMoveInput(const Uint8* input)
{
	if (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A])
	{
		animator->SetBool("walking", true);
		physics->velocity.x -= physics->horizontalSpeed;
		flip = SDL_FLIP_HORIZONTAL;
	}
	else if (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D])
	{
		animator->SetBool("walking", true);
		physics->velocity.x += physics->horizontalSpeed;
		flip = SDL_FLIP_NONE;
	}
	else
	{
		//TODO: Add friction
		physics->velocity.x = 0;
	}

	if (physics->velocity.x > physics->maxHorizontalSpeed)
		physics->velocity.x = physics->maxHorizontalSpeed;
	else if (physics->velocity.x < -physics->maxHorizontalSpeed)
		physics->velocity.x = -physics->maxHorizontalSpeed;
}

void Player::CheckJumpButton(const Uint8* input)
{
	physics->hadPressedJump = physics->pressingJumpButton;
	physics->pressingJumpButton = input[SDL_SCANCODE_X];
	//if (pressingJumpButton)
	//	std::cout << "!!!!" << std::endl;

	physics->canJump = ((!physics->hadPressedJump && physics->pressingJumpButton) && physics->jumpsRemaining > 0);

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

void Player::GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties)
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
	//TODO: Refactor this to use the physics component start position stuff
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
		" " << (int)layer << " " << impassable << std::endl;
}