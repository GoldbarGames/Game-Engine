#include "Player.h"
#include "Game.h"
#include <string>
#include "PhysicsComponent.h"
#include "Animator.h"
#include "Physics.h"
#include "HealthComponent.h"

Player::Player(const Vector2& pos) : Entity(pos)
{
	etype = "player";
	
	CreateCollider(0, 0, 20.25f, 41.40f);
	layer = DrawingLayer::COLLISION;
	drawOrder = 99;

	trigger = false;
	clickable = true;

	physics = new PhysicsComponent(this);
	physics->standAboveGround = true;
	physics->horizontalSpeed = 0.35f;
	physics->maxHorizontalSpeed = 0.35f;
	physics->canBePushed = true;

	// Initialize the spells here
	spell = Spell();

	//TODO: Pause all timers when game is paused
	timerSpellDebug.Start(1);
	doorTimer.Start(1);	
	timerSpellOther.Start(1);

	//rotation = glm::vec3(90.0f, 0.0f, 0.0f);
	health = new HealthComponent(10);
	health->showRelativeToCamera = true;
	health->showHealthBar = true;
	health->initialHealthBarScale = Vector2(200, 50);
	health->position = Vector2(1280 * 0.1f * Camera::MULTIPLIER, 720 * 0.1f * Camera::MULTIPLIER);
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

	spell.Render(renderer);
	
	if (closeRangeAttackCollider != nullptr)
	{
		if (renderer.game->debugMode && drawDebugRect)
		{
			if (debugSprite == nullptr)
				debugSprite = new Sprite(renderer.debugSprite->texture, renderer.debugSprite->shader);

			if (renderer.IsVisible(layer))
			{
				//TODO: Make this a function inside the renderer
				float rWidth = debugSprite->texture->GetWidth();
				float rHeight = debugSprite->texture->GetHeight();

				float targetWidth = closeRangeAttackCollider->bounds->w;
				float targetHeight = closeRangeAttackCollider->bounds->h;

				debugSprite->color = { 255, 255, 255, 255 };
				//debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));

				Vector2 colliderPosition = Vector2(position.x + closeRangeAttackCollider->offset.x, 
					position.y + closeRangeAttackCollider->offset.y);

				debugSprite->Render(colliderPosition, renderer);
			}
		}
	}
}

void Player::Render(const Renderer& renderer)
{
	Entity::Render(renderer);

	renderer.game->gui.healthComponents.push_back(health);
}

void Player::Update(Game& game)
{	
	static unsigned int count = 0;
	updatedAnimator = false;

	//TODO: Change this so that we collide with an object instead of hard-coding a number
	// Also, maybe draw an outline of the death barrier so the player can see where this is
	if (position.y > game.deathBarrierY || !health->IsAlive())
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
		if (!spell.isCasting)
		{
			UpdateNormally(game);
		}
		else
		{
			// Update Physics while we cast the spell
			if (physics->velocity.y < physics->CalcTerminalVelocity())
				physics->velocity.y += Physics::GRAVITY * game.dt;

			physics->CheckCollisions(game);
		}
			

		if (closeRangeAttackCollider != nullptr)
		{
			etype = "debug_missile";

			if (closeRangeAttackCollider->offset.x == 32 || closeRangeAttackCollider->offset.x == -32)
			{
				closeRangeAttackCollider->offset.x = 32 * scale.x;
				closeRangeAttackCollider->scale.x = 24 * scale.x;
			}

			closeRangeAttackCollider->CalculateCollider(position);

			for (unsigned int i = 0; i < game.entities.size(); i++)
			{
				game.collisionChecks++;

				Entity* entity = game.entities[i];

				if (entity->name == "crawler")
					int test = 0;

				if (entity == this)
					continue;

				SDL_Rect theirBounds = *(entity->GetBounds());
				theirBounds.w *= 2;
				theirBounds.x -= (theirBounds.w / 2);

				if (HasIntersection(*closeRangeAttackCollider->bounds, theirBounds))
				{
					if (entity->trigger)
					{
						Entity* us = this;
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
		if (currentSprite->HasAnimationElapsed())
		{
			delete_it(closeRangeAttackCollider);
			animator->SetBool("isCastingDebug", false);
		}			
	}

	if (spell.isCasting && currentSprite->HasAnimationElapsed())
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

	pressingUp = (input[SDL_SCANCODE_UP] || input[SDL_SCANCODE_W]);
	pressingDown = (input[SDL_SCANCODE_DOWN] || input[SDL_SCANCODE_S]);
	pressingLeft = (input[SDL_SCANCODE_LEFT] || input[SDL_SCANCODE_A]);
	pressingRight = (input[SDL_SCANCODE_RIGHT] || input[SDL_SCANCODE_D]);
	physics->hadPressedJump = physics->pressingJumpButton;
	physics->pressingJumpButton = input[SDL_SCANCODE_X];

	animator->SetBool("walking", false);
	animator->SetBool("holdingUp", pressingUp);
	animator->SetBool("holdingDown", pressingDown);

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
				game.cutscene->PlayCutscene(currentNPC->cutsceneLabel.c_str());
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
				game.SaveFile(game.currentSaveFileName);
			}
			else if (currentDoor != nullptr && doorTimer.HasElapsed())
			{
				if (currentDoor->name == "goal")
				{
					if (!currentDoor->isLocked)
					{
						game.state = GameState::LOAD_NEXT_LEVEL;
						game.nextLevel = currentDoor->nextLevelName;
						game.nextDoorID = currentDoor->destinationID;
						return;
					}
				}
				else
				{
					//TODO: Make this look better later
					// Should this play a cutscene here?
					if (!currentDoor->isLocked && animator->GetBool("isGrounded"))
					{
						SetPosition(currentDoor->GetDestination() + currentSprite->pivot);
						doorTimer.Start(500);
					}
				}


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

	if (!animator->GetBool("isHurt"))
	{
		//TODO: Should we limit the number that can be spawned?
		//TODO: Add a time limit between shots
		if (game.pressedDebugButton && timerSpellDebug.HasElapsed() && !animator->GetBool("isCastingDebug"))
		{
			CastSpellDebug(game, input);
		}
		else if (game.pressedSpellButton && timerSpellOther.HasElapsed() && !spell.isCasting && !animator->GetBool("isCastingSpell"))
		{
			spell.isCasting = spell.Cast(game);
			return;
		}
	}


	//TODO: What should happen if multiple buttons are pressed at the same time?
	if (game.pressedLeftTrigger)
	{
		spell.activeSpell--;
	}
	else if (game.pressedRightTrigger)
	{
		spell.activeSpell++;
	}

	if (spell.activeSpell < 0)
		spell.activeSpell = 0;

	//if (spell.activeSpell > spells.size() - 1)
	//	spell.activeSpell = spells.size() - 1;

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
		if (!animator->GetBool("holdingUp") && !animator->GetBool("holdingDown") 
			&& !animator->GetBool("isCastingDebug"))
		{
			GetMoveInput(input);
		}
		else if (physics->isGrounded)
		{
			physics->velocity.x = 0;
		}

		CheckJumpButton(input);

		// Update Physics
		if (physics->velocity.y < physics->CalcTerminalVelocity())
			physics->velocity.y += Physics::GRAVITY * game.dt;

		physics->CheckCollisions(game);
	}
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
	timerSpellOther.Start(animator->currentState->speed * (currentSprite->endFrame - currentSprite->startFrame));
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

		Missile* missile = game.SpawnMissile(missilePosition);
		if (missile != nullptr)
		{
			missile->SetVelocity(missileVelocity);
			// also set the angle here
			game.currentEther--;
			game.etherText->SetText("Ether: " + std::to_string(game.currentEther));
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
			closeRangeAttackCollider = new Collider(0, -32, 32, 16);
		}
		else if (pressingDown)
		{
			closeRangeAttackCollider = new Collider(0, 32, 32, 16);
		}
		else
		{
			closeRangeAttackCollider = new Collider(32 * scale.x, 0, 16 * scale.x, 32);
		}

		closeRangeAttackCollider->CalculateCollider(position);
	}

	if (currentSprite != nullptr)
	{
		currentSprite->ResetFrame();
		animator->SetBool("isCastingDebug", true);
		animator->Update(*this); // We need to update here in order to know how long to run the timer
		timerSpellDebug.Start(animator->currentState->speed * (currentSprite->endFrame - currentSprite->startFrame));
	}
	
	game.soundManager->PlaySound("se/shoot.wav", 1);	
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
		physics->velocity.x -= physics->horizontalSpeed;
		scale.x = -1;
	}
	else if (pressingRight)
	{
		animator->SetBool("walking", true);
		physics->velocity.x += physics->horizontalSpeed;
		scale.x = 1;
	}
	else
	{
		//physics->velocity.x = 0;
		physics->ApplyFriction(0.05f);
	}

	if (physics->velocity.x > physics->maxHorizontalSpeed)
		physics->velocity.x = physics->maxHorizontalSpeed;
	else if (physics->velocity.x < -physics->maxHorizontalSpeed)
		physics->velocity.x = -physics->maxHorizontalSpeed;
}

void Player::CheckJumpButton(const Uint8* input)
{
	//if (pressingJumpButton)
	//	std::cout << "!!!!" << std::endl;

	physics->canJump = ((!physics->hadPressedJump && physics->pressingJumpButton) && physics->jumpsRemaining > 0);
}


void Player::ResetPosition()
{
	position = physics->startPosition;
}

void Player::GetProperties(std::vector<Property*>& properties)
{
	Entity::GetProperties(properties);

	properties.emplace_back(new Property("Start Pos X", (int)physics->startPosition.x));
	properties.emplace_back(new Property("Start Pos Y", (int)physics->startPosition.y));
}

void Player::SetProperty(const std::string& key, const std::string& newValue)
{
	// 2. Based on the key, change its value
	//TODO: Refactor this to use the physics component start position stuff
	if (key == "Start Pos X")
	{
		if (newValue != "")
			physics->startPosition.x = std::stof(newValue);
	}
	else if (key == "Start Pos Y")
	{
		if (newValue != "")
			physics->startPosition.y = std::stof(newValue);
	}
}

void Player::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << 
		physics->startPosition.x << " " << physics->startPosition.y << " " << drawOrder <<
		" " << (int)layer << " " << impassable << std::endl;
}