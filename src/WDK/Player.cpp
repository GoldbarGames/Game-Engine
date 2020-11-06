#include "Player.h"
#include "../ENGINE/Game.h"
#include <string>
#include "../ENGINE/PhysicsComponent.h"
#include "../ENGINE/Animator.h"
#include "../ENGINE/AnimatorInfo.h"
#include "../ENGINE/Physics.h"
#include "../ENGINE/HealthComponent.h"
#include "../ENGINE/Text.h"
#include "../ENGINE/Sprite.h"
#include "../ENGINE/Renderer.h"
#include "../ENGINE/CutsceneManager.h"
#include "Ladder.h"
#include "Door.h"
#include "NPC.h"
#include "Decoration.h"
#include "../ENGINE/SoundManager.h"
#include "../ENGINE/Property.h"
#include "Missile.h"
#include "MyGUI.h"
#include "../ENGINE/FileManager.h"

Player::Player(const Vector2& pos) : MyEntity(pos)
{
	etype = "player";
	
	CreateCollider(0, 0, 20.25f, 41.40f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 99;

	trigger = false;
	clickable = true;

	spell.player = this;

	physics = neww PhysicsComponent(this);
	physics->standAboveGround = true;
	physics->horizontalSpeed = 0.35f;
	physics->maxHorizontalSpeed = 0.35f;
	physics->canBePushed = true;
	physics->canBePickedUp = true;

	//TODO: Pause all timers when game is paused
	timerSpellDebug.Start(1);
	doorTimer.Start(1);	
	timerSpellOther.Start(1);

	//rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	health = neww HealthComponent(10);
	//health->showRelativeToCamera = true;
	health->showHealthBar = true;
	health->initialHealthBarScale = Vector2(40, 10);
	//health->initialHealthBarScale = Vector2(200, 50);
	//health->position = Vector2(1280 * 0.1f * Camera::MULTIPLIER, 720 * 0.1f * Camera::MULTIPLIER);

	//health->showHealthIcons = true;
	//health->iconPath = "assets/gui/heart.png";
	//health->position = Vector2(1280 * 0.8f * Camera::MULTIPLIER, 720 * 0.9f * Camera::MULTIPLIER);
}

Player::~Player()
{

}

void Player::OnClickPressed(Uint32 mouseState, Game& game)
{
	std::cout << "Clicked, pressed down on " << etype << "!" << std::endl;
}

void Player::RenderDebug(const Renderer& renderer)
{
	Entity::RenderDebug(renderer);

	// NOTE: These are not aligned properly, origin not at center
	//renderer.RenderDebugRect(physics->newBoundsVertical, Vector2(1, 1), { 255, 0, 0, 255 });
	//renderer.RenderDebugRect(physics->floorBounds, Vector2(1, 1), {0, 255, 0, 255 });

	spell.RenderDebug(renderer);
	
	if (closeRangeAttackCollider != nullptr)
	{
		if (renderer.game->debugMode && drawDebugRect)
		{
			if (renderer.IsVisible(layer))
			{
				SDL_Rect rect;
				rect.x = position.x + closeRangeAttackCollider->offset.x;
				rect.y = position.y + closeRangeAttackCollider->offset.y;
				rect.w = closeRangeAttackCollider->bounds->w;
				rect.h = closeRangeAttackCollider->bounds->h;

				renderer.RenderDebugRect(rect, scale);
			}
		}
	}
}

void Player::Render(const Renderer& renderer)
{
	Entity::Render(renderer);

	if (!isDouble)
	{
		if (gui == nullptr)
		{
			gui = static_cast<MyGUI*>(renderer.game->gui);
		}

		gui->playerSpell = &spell;
		gui->healthComponents.push_back(health);
	}
}

void Player::Update(Game& game)
{	
	static unsigned int count = 0;
	updatedAnimator = false;

	currentSprite.color = color;
	health->position = position + Vector2(0, -50);

	//TODO: Change this so that we collide with an object instead of hard-coding a number
	// Also, maybe draw an outline of the death barrier so the player can see where this is
	if (position.y > game.deathBarrierY || !health->IsAlive())
	{
		if (!isDouble)
		{
			game.state = GameState::RESET_LEVEL;
			return;
		}
	}		

	if (game.cutsceneManager.watchingCutscene)
	{
		//TODO: Get input for handling the textbox
		animator->SetBool("holdingUp",  false);
	}
	else
	{
		UpdateNormally(game);

		if (!isDouble)
		{
			spell.Update(game);
		}

		// Check if an enemy has been hit by our attack
		if (closeRangeAttackCollider != nullptr)
		{
			etype = "debug_missile";

			if (closeRangeAttackCollider->offset.x == 32 || closeRangeAttackCollider->offset.x == -32)
			{
				closeRangeAttackCollider->offset.x = 32 * scale.x;
				closeRangeAttackCollider->scale.x = 24 * scale.x;
			}

			closeRangeAttackCollider->CalculateCollider(position, rotation);

			// Check if an enemy has been hit by our attack
			for (unsigned int i = 0; i < game.entities.size(); i++)
			{
				game.collisionChecks++;

				MyEntity* entity = dynamic_cast<MyEntity*>(game.entities[i]);
				if (entity == nullptr || entity == this)
					continue;

				SDL_Rect theirBounds = *(entity->GetBounds());
				theirBounds.w *= 2;
				theirBounds.x -= (theirBounds.w / 2);

				if (HasIntersection(*closeRangeAttackCollider->bounds, theirBounds))
				{
					if (entity->trigger)
					{
						MyEntity* us = this;
						entity->OnTriggerStay(*us, game);
					}
				}
			}

			etype = "player";
		}
	}

	//std::cout << "--" << count << "--" << std::endl;
	//std::cout << GetPosition() << std::endl;

	//count++;
	
	UpdateAnimator();
}

void Player::UpdateAnimator()
{
	if (updatedAnimator)
		return;

	updatedAnimator = true;

	// TODO: Refactor this
	if (animator->GetBool("isHurt") && animator->animationTimer.HasElapsed())
	{
		animator->SetBool("isHurt", false);
	}

	if (animator->GetBool("isCastingDebug"))
	{		
		if (currentSprite.HasAnimationElapsed())
		{
			delete_it(closeRangeAttackCollider);
			animator->SetBool("isCastingDebug", false);
		}			
	}

	if (spell.isCasting && currentSprite.HasAnimationElapsed())
	{
		animator->SetBool("isCastingSpell", false);
		spell.isCasting = false;
	}

	if (animator != nullptr)
		animator->Update(*this);
}

void Player::UpdateNormally(Game& game)
{
	//Set texture based on current keystate
	const Uint8* input = SDL_GetKeyboardState(NULL);

	bool wasHoldingUp = animator->GetBool("holdingUp");

	if (isDouble)
	{
		pressingDown = (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W]);
		pressingUp = (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S]);
		pressingRight = (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A]);
		pressingLeft = (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D]);
	}
	else
	{
		pressingUp = (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W]);
		pressingDown = (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S]);
		pressingLeft = (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A]);
		pressingRight = (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D]);
	}

	physics->hadPressedJump = physics->pressingJumpButton;
	physics->pressingJumpButton = input[SDL_SCANCODE_X];
	pressingRun = input[SDL_SCANCODE_Z];

	// Press or hold the button down to cast a spell
	game.pressedSpellButton = input[SDL_SCANCODE_V];

	animator->SetBool("isRunning", false);
	animator->SetBool("walking", false);
	animator->SetBool("holdingUp", pressingUp);
	animator->SetBool("holdingDown", pressingDown);
	animator->SetBool("isHoldingSpellButton", game.pressedSpellButton);

	if (animator->currentState->name == "push_end")
	{
		if (currentSprite.currentFrame == currentSprite.endFrame)
		{
			animator->SetBool("timerElapsedPushEnd", true);
		}		
	}

	if (currentLadder != nullptr && !physics->hadPressedJump && physics->pressingJumpButton)
	{
		GetAnimator()->SetBool("onLadder", false);
		currentLadder = nullptr;
	}

	// if we are holding up
	if (pressingUp)
	{
		// for when up is pressed, not held
		// if we are in front of a door, ladder, or NPC...
		if (!wasHoldingUp)
		{
			if (currentNPC != nullptr)
			{
				game.cutsceneManager.PlayCutscene(currentNPC->cutsceneLabel.c_str());
			}
			else if (currentDecoration != nullptr)
			{
				game.cutsceneManager.PlayCutscene(currentDecoration->cutsceneLabel.c_str());
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

					// snap player to center of the ladder
					position.x = currentLadder->position.x;
				}
			}
			else if (currentCheckpoint != nullptr)
			{
				game.fileManager->SaveFile(game.currentSaveFileName);
			}
			else if (currentDoor != nullptr && doorTimer.HasElapsed())
			{
				// Entering a door loads a new level
				if (!currentDoor->isLocked)
				{
					game.soundManager.PlaySound("se/door1.wav", 0);
					game.state = GameState::LOAD_NEXT_LEVEL;
					game.nextLevel = currentDoor->nextLevelName;
					game.nextDoorID = currentDoor->destinationID;
					return;
				}

				/*
				* 
				*  Old code for using doors to teleport around a level
				* 
				if (currentDoor->name == "goal")
				{
					
				}
				else
				{
					//TODO: Make this look better later
					// Should this play a cutscene here?
					if (!currentDoor->isLocked && animator->GetBool("isGrounded"))
					{
						SetPosition(currentDoor->GetDestination() + currentSprite.pivot);
						doorTimer.Start(500);
					}
				}
				*/

			}
		}

		// for when up is being held
		if (currentLadder != nullptr)
		{
			// TODO: What if there is a door and a ladder at the same spot?
			if (currentLadder->GetAnimator()->currentState->name != "top")
			{
				physics->velocity.y = 0;
				physics->velocity.x = 0;
				animator->SetBool("onLadder", true);
			}
		}		
	}

	if (!isDouble)
	{
		if (!animator->GetBool("isHurt"))
		{
			//TODO: Should we limit the number that can be spawned?
			//TODO: Add a time limit between shots
			if (game.pressedDebugButton && timerSpellDebug.HasElapsed() 
				&& !animator->GetBool("isCastingDebug"))
			{
				CastSpellDebug(game, input);
			}
			else if (game.pressedSpellButton && timerSpellOther.HasElapsed() && 
				!spell.isCasting && !animator->GetBool("isCastingSpell"))
			{
				spell.isCasting = spell.Cast(game);
				game.soundManager.PlaySound("se/push.wav", 1);
			}
		}

		// If we're not currently casting a spell,
		// allow the player to switch spells
		if (!spell.isCasting)
		{
			spell.CycleSpells(game);
		}		
	}

	if (canMove)
	{
		// If on the ladder, only move up or down
		if (animator->GetBool("onLadder"))
		{
			GetLadderInput(input);
		}
		else
		{
			// Don't move if we are casting debug, or looking up/down
			if (!pressingUp && !pressingDown && !animator->GetBool("isCastingDebug"))
			{
				GetMoveInput(input);
			}
			else if (physics->isGrounded)
			{
				physics->velocity.x = 0;
			}

			// Update Physics
			if (physics->velocity.y < physics->CalcTerminalVelocity())
				physics->velocity.y += Physics::GRAVITY * game.dt;
		}

		CheckJumpButton(input);
	}	
	else
	{
		if (timerSpellOther.HasElapsed())
		{
			canMove = true;
		}
	}

	physics->CheckCollisions(game);
}

void Player::CastSpellDebug(Game &game, const Uint8* input)
{
	if (game.currentEther <= 0)
		return;

	bool createMissile = false;

	if (createMissile)
	{
		Vector2 missilePosition = this->position;
		//missilePosition.x += (this->currentSprite->GetRect()->w / 2);
		missilePosition.y += (this->collider->bounds->h / 2);

		const float missileSpeed = 0.25f;
		Vector2 missileVelocity = Vector2(0, 0);
		float angle = 0;

		if (pressingUp)
		{
			missileVelocity.y = -missileSpeed;
			angle = 270;
		}
		else if (pressingDown)
		{
			missileVelocity.y = missileSpeed;
			angle = 90;
		}
		else if (scale.x > 0)
		{
			missileVelocity.x = missileSpeed;
		}
		else if (scale.x < 0)
		{
			missileVelocity.x = -missileSpeed;
			angle = 180;
		}

		Missile* missile = static_cast<Missile*>(game.SpawnEntity("missile", missilePosition, 0));

		missile->Init(game, "debug");
		if (missile != nullptr)
		{
			missile->SetVelocity(missileVelocity);
			// also set the angle here
			game.currentEther--;
			//game.gui.texts["ether"]->SetText("Ether: " + std::to_string(game.currentEther));
			missile->etype = "debug_missile";						
		}
	}
	else
	{
		if (closeRangeAttackCollider != nullptr)
		{
			delete_it(closeRangeAttackCollider);
		}

		if (pressingUp)
		{
			closeRangeAttackCollider = neww Collider(0, -32, 32, 16);
		}
		else if (pressingDown)
		{
			closeRangeAttackCollider = neww Collider(0, 32, 32, 16);
		}
		else
		{
			closeRangeAttackCollider = neww Collider(32 * scale.x, 0, 16 * scale.x, 32);
		}

		closeRangeAttackCollider->CalculateCollider(position, rotation);
	}

	currentSprite.ResetFrame();
	animator->SetBool("isCastingDebug", true);
	animator->Update(*this); // We need to update here in order to know how long to run the timer
	timerSpellDebug.Start(animator->currentState->speed * (currentSprite.endFrame - currentSprite.startFrame));
	
	game.soundManager.PlaySound("se/shoot.wav", 1);	
}

void Player::GetLadderInput(const Uint8* input)
{
	animator->SetBool("climbing", false);
	
	if (pressingUp && currentLadder != currentLadder->top)
	{
		animator->SetBool("climbing", true);
		physics->velocity.y -= physics->horizontalSpeed * 0.5f;
	}
	else if (pressingDown)
	{
		animator->SetBool("climbing", true);
		physics->velocity.y += physics->horizontalSpeed * 0.5f;
	}
	else
	{
		physics->velocity.y = 0;
	}

	if (pressingLeft)
	{
		animator->SetBool("climbing", true);
		physics->velocity.x -= physics->horizontalSpeed * 0.15f;
		scale.x = -1;
	}
	else if (pressingRight)
	{
		animator->SetBool("climbing", true);
		physics->velocity.x += physics->horizontalSpeed * 0.15f;
		scale.x = 1;
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
	if (pressingLeft)
	{
		animator->SetBool("walking", true);
		animator->SetBool("isRunning", pressingRun);
		physics->velocity.x -= physics->horizontalSpeed;
		
		if (!spell.isShieldUp)
		{
			scale.x = spell.isShort ? -spell.SHRINK_SIZE : -1.0f;
		}
	}
	else if (pressingRight)
	{
		animator->SetBool("walking", true);
		animator->SetBool("isRunning", pressingRun);
		physics->velocity.x += physics->horizontalSpeed;

		if (!spell.isShieldUp)
		{
			scale.x = spell.isShort ? spell.SHRINK_SIZE : 1.0f;
		}
	}
	else
	{
		//physics->velocity.x = 0;
		animator->SetBool("isRunning", false);

		// If we are on a moving platform, set velocity to zero, no friction
		if (physics->parent != nullptr)
		{
			physics->velocity.x = 0;
		}
		else
		{
			physics->ApplyFriction(0.05f);
		}
	}

	float runFactor = 0.8f;
	if (pressingRun)
	{
		runFactor = 1.2f;
	}

	if (physics->velocity.x > runFactor * physics->maxHorizontalSpeed)
		physics->velocity.x = runFactor * physics->maxHorizontalSpeed;
	else if (physics->velocity.x < runFactor  * -physics->maxHorizontalSpeed)
		physics->velocity.x = runFactor  * -physics->maxHorizontalSpeed;
}

void Player::CheckJumpButton(const Uint8* input)
{
	//if (pressingJumpButton)
	//	std::cout << "!!!!" << std::endl;

	physics->canJump = ((!physics->hadPressedJump && physics->pressingJumpButton) && physics->jumpsRemaining > 0);
}


void Player::ResetPosition()
{
	position = startPosition;
}


void Player::UpdateSpellAnimation(const char* spellName)
{
	// 1. Prevent the player from leaving this state and pressing any other buttons during this time
	animator->SetBool("isCastingSpell", true);

	// 2. Set the player's animation to the PUSH spell casting animation
	animator->SetState(spellName);

	// 3. Actually set the player's sprite to the casting sprite
	animator->Update(*this);

	// 4. Set the timer to the length of the casting animation
	timerSpellOther.Start(animator->currentState->speed *
		(GetSprite()->endFrame - GetSprite()->startFrame));
}

void Player::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Start Pos X", (int)startPosition.x));
	properties.emplace_back(new Property("Start Pos Y", (int)startPosition.y));
}

void Player::SetProperty(const std::string& key, const std::string& newValue)
{
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

void Player::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);
}

void Player::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);
}