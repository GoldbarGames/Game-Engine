#include "Spell.h"
#include "Game.h"
#include "PhysicsComponent.h"
#include "Tree.h"
#include "Missile.h"
#include "SpriteManager.h"
#include "Renderer.h"
#include "Sprite.h"
#include "Player.h"
#include "RandomManager.h"
#include "Editor.h"
#include "Timer.h"

typedef bool (Spell::* SpellFunction)(Game& game);

struct SpellLUT {
	char name[32];
	SpellFunction method;
};

std::vector<SpellLUT> spellFunctions = {
	{"push", &Spell::CastPush},
	{"pop", &Spell::CastPop},
	{"float", &Spell::CastFloat},
	{"freeze", &Spell::CastFreeze},
	{"carry", &Spell::CastCarry},
	{"protect", &Spell::CastProtect},
	{"return", &Spell::CastReturn},
	{"seed", &Spell::CastSeed},
	{"double", &Spell::CastDouble },
	{"short", &Spell::CastShort },
	{"flash", &Spell::CastFlash },
	{"break",&Spell::CastBreak },
	{"search",&Spell::CastSearch },
	{"turbo",&Spell::CastTurbo },
	{"crypt",&Spell::CastCrypt },
	{"sleep", &Spell::CastSleep }
};


Spell::Spell() 
{ 
	names = { "push", "pop", "float", "freeze", "carry", "protect",
		"return", "seed", "double", "short", "flash", "break",
		"search", "turbo", "crypt", "sleep" };
}

Spell::~Spell()
{
	//TODO: Maybe store these in the Game instead
	// so that they stay loaded across all levels
	for (int i = 0; i < spellIcons.size(); i++)
	{
		if (spellIcons[i] != nullptr)
			delete_it(spellIcons[i]);
	}

	spellIcons.clear();
}

void Spell::CycleSpells(Game& game)
{
	//TODO: What should happen if multiple buttons are pressed at the same time?
	if (game.pressedLeftTrigger)
	{
		activeSpell--;
	}
	else if (game.pressedRightTrigger)
	{
		activeSpell++;
	}

	if (activeSpell < 0)
		activeSpell = activeSpell = names.size() - 1;

	if (activeSpell > names.size() - 1)
		activeSpell = 0;
}

void Spell::RenderDebug(const Renderer& renderer)
{
	if (isCasting)
	{
		renderer.RenderDebugRect(spellRangeRect, Vector2(1, 1));
	}
}

void Spell::Render(const Renderer& renderer)
{
	if (activeSpell >= 0 && activeSpell < spellIcons.size())
	{
		spellIcons[activeSpell]->Render(Vector2(100 * Camera::MULTIPLIER, 600 * Camera::MULTIPLIER), renderer);
	}
}

void Spell::Update(Game& game)
{
	if (spellIcons.size() == 0)
	{
		std::cout << "SPELL ICONS " << spellIcons.size() << std::endl;
		std::cout << "SPELL NAMES " << names.size() << std::endl;
		for (int i = 0; i < names.size(); i++)
		{
			Texture* texture = game.spriteManager->GetImage("assets/gui/icon/icon_" + names[i] + ".png");
			Sprite* sprite = neww Sprite(texture, game.renderer->shaders[ShaderName::Default]);
			sprite->keepPositionRelativeToCamera = true;
			sprite->keepScaleRelativeToCamera = true;
			std::cout << i << std::endl;
			spellIcons.push_back(sprite);
		}
	}

	game.player->GetAnimator()->SetBool("isMovingForward", 
		(game.player->scale.x > 0 && game.player->physics->velocity.x > 0));

	bool wasCast = false;

	// Continue to grow the seed even when we are not actively casting the spell
	if (isGrowingSeed && timer.HasElapsed())
	{
		for (const auto& func : spellFunctions)
		{
			if (func.name == names[activeSpell])
			{
				wasCast = (this->*func.method)(game);
			}
		}				
	}

	if (isCasting)
	{
		for (const auto& func : spellFunctions)
		{
			// Exclude some spells from being cast every frame
			if (func.name == names[activeSpell])
			{
				if (names[activeSpell] == "push"
					|| names[activeSpell] == "short")
				{
					wasCast = (this->*func.method)(game);
				}
				else if (names[activeSpell] == "pop"
					|| names[activeSpell] == "float"
					|| names[activeSpell] == "carry"
					|| names[activeSpell] == "double"
					|| names[activeSpell] == "freeze")
				{
					// This is used to spawn the missile at a certain frame in the animation
					// rather than spawning the missile instantly at the initial frame
					if (game.player->GetSprite()->currentFrame >= specialFrame)
					{
						wasCast = (this->*func.method)(game);
						specialFrame = 9999;
					}

					if (wasCast)
					{
						game.SortEntities(game.entities);
					}
				}
				else if (names[activeSpell] == "return")
				{
					if (game.player->GetSprite()->currentFrame >= specialFrame)
					{
						wasCast = (this->*func.method)(game);
						specialFrame = 8000;
					}
					else if (specialFrame == 8000 && game.player->timerSpellOther.HasElapsed())
					{
						game.player->GetAnimator()->SetBool("returned", false);
						game.player->GetAnimator()->SetBool("isCasting", false);	
						game.player->UpdateSpellAnimation("idle");
						game.player->GetSprite()->ResetFrame();
						specialFrame = 9999;
					}
				}
			}
		}
	}

}


bool Spell::Cast(Game& game)
{
	bool success = false;

	// Clear out the list of entities being affected 
	// by the current casting of the current spell
	affectedEntities.clear();

	// Stop moving horizontally at the start of casting a spell
	game.player->physics->velocity.x = 0;

	game.player->UpdateSpellAnimation(names[activeSpell].c_str());

	if (names[activeSpell] != "protect")
	{
		game.player->GetSprite()->ResetFrame();
	}

	for (const auto& func : spellFunctions)
	{
		if (func.name == names[activeSpell])
		{
			success = (this->*func.method)(game);
		}
	}

	if (success)
	{
		game.SortEntities(game.entities);
	}

	return success;
}

bool Spell::CastPush(Game& game)
{
	// Create a rectangle collider in front of the player (direction facing)
	spellRangeRect.x = (int)game.player->position.x;
	spellRangeRect.y = (int)game.player->position.y;
	spellRangeRect.w = 64;
	spellRangeRect.h = 52;

	int DISTANCE_FROM_CENTER_X = 90;
	int DISTANCE_FROM_CENTER_Y = 0;

	// Add distance to the center so that it covers the entire cloud of wind
	if (game.player->scale.x < 0)
	{
		//spellRangeRect.w *= -1;
		spellRangeRect.x -= DISTANCE_FROM_CENTER_X;
	}
	else
	{
		spellRangeRect.x += DISTANCE_FROM_CENTER_X;
	}

	spellRangeRect.y += DISTANCE_FROM_CENTER_Y;

	//std::cout << "Rect for push spell:" << std::endl;
	//std::cout << "(" << spellRange->x << "," << spellRange->y << "," <<
	//	spellRange->w << "," << spellRange->h << ")" << std::endl;
	//game.debugRectangles.push_back(spellRange);

	const float PUSH_SPEED = 0.5f;

	Vector2 pushVelocity = Vector2(PUSH_SPEED, 0.0f);
	if (game.player->scale.x < 0)
		pushVelocity = Vector2(-1 * PUSH_SPEED, 0.0f);

	SDL_Rect newRectOurs, newRectTheirs;
	newRectOurs = ConvertCoordsFromCenterToTopLeft(spellRangeRect);

	// 4. If the collider intersects with anything that can be pushed,
	for (unsigned int i = 0; i < game.entities.size(); i++)
	{
		bool alreadyAffectedBySpell = false;
		for (int k = 0; k < affectedEntities.size(); k++)
		{
			if (game.entities[i] == affectedEntities[k])
			{
				alreadyAffectedBySpell = true;
				break;
			}
		}

		if (alreadyAffectedBySpell)
		{
			continue;
		}

		const SDL_Rect* theirBounds = game.entities[i]->GetBounds();
		newRectTheirs = ConvertCoordsFromCenterToTopLeft(*theirBounds);

		if (HasIntersection(newRectOurs, newRectTheirs))
		{
			//TODO: Is there a better way to do this than to check the type?
			Entity* entity = game.entities[i];
			if (entity->physics != nullptr)
			{
				// 5. Then make that object move until it hits a wall
				if (entity->physics->canBePushed)
				{
					entity->physics->Push(pushVelocity);
					affectedEntities.push_back(entity);
				}				
			}
			else if (entity->etype == "tree")
			{
				// TODO: Maybe do this differently?
				affectedEntities.push_back(entity);
				entity->GetAnimator()->SetBool("isPushed", true);
			}
		}
	}

	return true;
}

bool Spell::CastPop(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 2;
		return true;
	}

	Vector2 offset = Vector2(game.player->scale.x < 0 ? -16 : 16, 0);
	
	// TODO: Don't create more than one fireball at a time
	Missile* fireball = static_cast<Missile*>(game.SpawnEntity("missile", game.player->position + offset, 0));

	if (fireball == nullptr)
	{
		std::cout << "ERROR CASTING POP SPELL: CREATING FIREBALL" << std::endl;
		return false; // failed to create fireball
	}

	fireball->Init("pop");
	fireball->SetVelocity(Vector2(game.player->scale.x < 0 ? -0.25f : 0.25f, -1.0f));

	return true;
}

bool Spell::CastFloat(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 7;
		return true;
	}

	Vector2 offset = Vector2(game.player->scale.x < 0 ? -16 : 16, 0);

	// TODO: Don't create more than one fireball at a time
	Missile* bubble = static_cast<Missile*>(game.SpawnEntity("missile", game.player->position + offset, 0));

	if (bubble == nullptr)
	{
		std::cout << "ERROR CASTING FLOAT SPELL: CREATING BUBBLE" << std::endl;
		return false; // failed to create missile
	}

	bubble->Init("float");
	bubble->SetVelocity(Vector2(game.player->scale.x < 0 ? -0.25f : 0.25f, 0.0f));

	return true;
}

bool Spell::CastFreeze(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 5;
		return true;
	}

	Vector2 offset = Vector2(game.player->scale.x < 0 ? -64 : 64, 16);

	// TODO: Don't create more than one fireball at a time
	Missile* iceMissile = static_cast<Missile*>(game.SpawnEntity("missile", game.player->position + offset, 0));

	if (iceMissile == nullptr)
	{
		std::cout << "ERROR CASTING FREEZE SPELL: CREATING ICE" << std::endl;
		return false; // failed to create missile
	}

	iceMissile->Init("freeze");
	iceMissile->SetVelocity(Vector2(game.player->scale.x < 0 ? -0.25f : 0.25f, 0.0f));

	if (game.player->scale.x > 0)
		iceMissile->scale.x = 1.0f;
	else
		iceMissile->scale.x = -1.0f;

	return true;
}

bool Spell::CastFlash(Game& game)
{
	return true;
}

bool Spell::CastDouble(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 12;
		return true;
	}

	if (playerClone != nullptr)
	{
		delete playerClone;
		playerClone = nullptr;
	}
	
	Vector2 offset = Vector2(game.player->scale.x > 0 ? -64 : 64, -16);
	Vector2 clonePosition = game.player->position + offset;
	playerClone = static_cast<Player*>(game.SpawnEntity("player", clonePosition, 0));

	// If not successful, error out
	if (playerClone == nullptr)
	{
		std::cout << "ERROR CASTING DOUBLE SPELL: CREATING CLONE" << std::endl;
		return false;
	}	

	playerClone->isDouble = true;
	playerClone->SetColor({ 94, 206, 255, 128 });
	return true;
}

bool Spell::CastShort(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 5000;

		if (std::abs(game.player->scale.x) < 1.0f)
		{
			isShort = true;
			game.player->UpdateSpellAnimation("short_grow");
			game.player->GetAnimator()->SetBool("isShort", false);
		}
		else
		{
			isShort = false;
			game.player->UpdateSpellAnimation("short");
			game.player->GetAnimator()->SetBool("isShort", true);
		}

		return true;
	}

	bool isFacingRight = game.player->scale.x > 0;
	int multiplier = isFacingRight ? 1 : -1;

	if (!isShort)
	{
		if (std::abs(game.player->scale.x) > SHRINK_SIZE)
		{
			LerpVector2(game.player->scale, Vector2(multiplier * SHRINK_SIZE, SHRINK_SIZE), 0.05f, 0.025f);
			game.player->CreateCollider(0, 0, 20.25f * std::abs(game.player->scale.x), 41.40f * game.player->scale.y);
		}
		else
		{
			specialFrame = 9999;	
			game.player->CreateCollider(0, 0, 2.025f, 4.140f);
		}
	}
	else
	{
		if (std::abs(game.player->scale.x) < 1.0f)
		{
			LerpVector2(game.player->scale, Vector2(multiplier * 1.0f, 1.0f), 0.05f, 0.025f);
			game.player->CreateCollider(0, 0, 20.25f * std::abs(game.player->scale.x), 41.40f * game.player->scale.y);
		}
		else
		{
			specialFrame = 9999;
			game.player->CreateCollider(0, 0, 20.25f, 41.40f);
		}
	}

	return true;
}

bool Spell::CastProtect(Game& game)
{
	isShieldUp = !isShieldUp;

	game.player->GetAnimator()->SetBool("isShieldUp", isShieldUp);
	game.player->UpdateSpellAnimation(names[activeSpell].c_str());
	//game.player->GetSprite()->ResetFrame();

	return true;
}

bool Spell::CastReturn(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 20;
		return true;
	}
	else
	{
		game.player->position = game.player->startPosition;
		game.player->GetAnimator()->SetBool("returned", true);
		game.player->UpdateSpellAnimation("return_exit");
		game.player->GetSprite()->ResetFrame();
	}

	return true;
}

bool Spell::CastSeed(Game& game)
{
	static Vector2 spawnBeanstalkPosition = game.player->position;

	static int currentPartNumber = 0;
	static int startSuffix = 0;
	std::string suffix = std::to_string(startSuffix);

	if (isPlantedSeed)
	{
		timer.Reset();

		const int numberOfPieces = 6;
		const std::string b_bottom = "b_bottom";
		const std::string b_middle = "b_middle";
		const std::string b_top = "b_top";

		Entity* currentLadder = nullptr;

		// Grow the starting part here
		if (currentPartNumber == -1)
		{
			currentPartNumber++;
			game.player->UpdateSpellAnimation("seed_grow");
			game.player->GetSprite()->ResetFrame();
			isGrowingSeed = true;

			// Remove any existing beanstalks
			for (int i = 0; i < beanstalkParts.size(); i++)
			{
				beanstalkParts[i]->shouldDelete = true;
			}
			beanstalkParts.clear();

			startSuffix = game.randomManager->RandomRange(1, 2);
			std::string suffix = std::to_string(startSuffix);

			// Spawn the neww beanstalk
			currentLadder = game.SpawnEntity("ladder", spawnBeanstalkPosition, 1);

			if (currentLadder != nullptr)
			{
				currentLadder->GetAnimator()->SetState((b_bottom + suffix).c_str());
				currentLadder->GetAnimator()->DoState(*currentLadder);
				beanstalkParts.push_back(currentLadder);
			}
			else // If the root of the beanstalk can't be spawned, do nothing
			{
				isPlantedSeed = !isPlantedSeed;
				isGrowingSeed = false;
			}
		}
		else
		{
			// Work our way from bottom to the top
			spawnBeanstalkPosition.y -= TILE_SIZE * Camera::MULTIPLIER;
			currentLadder = game.SpawnEntity("ladder", spawnBeanstalkPosition, 1);

			if (startSuffix == 2)
				suffix = (currentPartNumber % 2 == 0) ? "2" : "1";
			else
				suffix = (currentPartNumber % 2 != 0) ? "2" : "1";

			if (currentLadder != nullptr)
			{
				currentLadder->GetAnimator()->SetState((b_middle + suffix).c_str());
				currentLadder->GetAnimator()->DoState(*currentLadder);
				beanstalkParts.push_back(currentLadder);
				currentPartNumber++;
			}
			else // If the beanstalk can't be spawned, do nothing
			{
				currentLadder = beanstalkParts.back();
				currentPartNumber = numberOfPieces;
			}		

			if (currentPartNumber >= numberOfPieces)
			{
				// Change the sprite of the top part
				if (startSuffix == 2)
					suffix = (currentPartNumber % 2 == 0) ? "2" : "1";
				else
					suffix = (currentPartNumber % 2 != 0) ? "2" : "1";

				currentLadder = beanstalkParts.back();
				currentLadder->GetAnimator()->SetState((b_top + suffix).c_str());
				currentLadder->GetAnimator()->DoState(*currentLadder);
				isPlantedSeed = !isPlantedSeed;
				isGrowingSeed = false;
			}
		}

	}
	else
	{
		// Plant the seed at the nearest tile to the player
		spawnBeanstalkPosition = game.player->position;
		spawnBeanstalkPosition.x -= game.renderer->camera.position.x;
		spawnBeanstalkPosition.y -= game.renderer->camera.position.y;
		spawnBeanstalkPosition = game.CalculateObjectSpawnPosition(spawnBeanstalkPosition, game.editor->GRID_SIZE);
		currentPartNumber = -1;
		isPlantedSeed = !isPlantedSeed;
		timer.Start(100);
	}	

	return true;
}

bool Spell::CastBreak(Game& game)
{
	return true;
}

bool Spell::CastSearch(Game& game)
{
	return true;
}

bool Spell::CastTurbo(Game& game)
{
	return true;
}

bool Spell::CastCrypt(Game& game)
{
	return true;
}

bool Spell::CastCarry(Game& game)
{
	if (specialFrame == 9999)
	{
		specialFrame = 5;
		return true;
	}

	if (carryMissile != nullptr)
	{
		carryMissile->GetAnimator()->SetBool("destroyed", true);
		
		if (carryMissile->pickedUpEntity != nullptr)
		{
			carryMissile->pickedUpEntity->physics->isPickedUp = true;
			carryMissile->pickedUpEntity = nullptr;
		}

		carryMissile = nullptr;
	}
	else
	{
		Vector2 offset = Vector2(game.player->scale.x < 0 ? -16 : 16, 0);

		// TODO: Don't create more than one at a time
		carryMissile = static_cast<Missile*>(game.SpawnEntity("missile", game.player->position + offset, 0));

		if (carryMissile == nullptr)
		{
			std::cout << "ERROR CASTING CARRY SPELL: CREATING HAND" << std::endl;
			return false; // failed to create
		}

		carryMissile->selfPointer = &carryMissile;

		carryMissile->Init("carry");
		carryMissile->SetVelocity(Vector2(game.player->scale.x < 0 ? -0.15f : 0.15f, 0.0f));

		if (game.player->scale.x > 0)
			carryMissile->scale.x = 1.0f;
		else
			carryMissile->scale.x = -1.0f;
	}

	return true;
}

bool Spell::CastRead(Game& game)
{
	return true;
}

bool Spell::CastTouch(Game& game)
{
	return true;
}

bool Spell::CastJump(Game& game)
{
	return true;
}

bool Spell::CastSleep(Game& game)
{
	return true;
}