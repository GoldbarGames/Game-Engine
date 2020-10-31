#include "Enemy.h"
#include "../ENGINE/Entity.h"
#include "Player.h"
#include "../ENGINE/Game.h"
#include "../ENGINE/Collider.h"
#include "../ENGINE/Timer.h"
#include "../ENGINE/Vector2.h"
#include "../ENGINE/PhysicsComponent.h"
#include "../ENGINE/HealthComponent.h"
#include <algorithm>
#include "../ENGINE/Sprite.h"
#include "../ENGINE/Renderer.h"
#include "MyGUI.h"

Enemy::Enemy(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::COLLISION;
	drawOrder = 90;
	etype = "enemy";
	trigger = true;

	CreateCollider(0, -4, 54, 32);

	physics = neww PhysicsComponent(this);
	physics->useGravity = true;
	physics->canBePushed = true;
	physics->canBePickedUp = true;
	startPosition = pos;
	physics->standAboveGround = true;

	health = neww HealthComponent(4);
	health->showHealthBar = true;
	health->initialHealthBarScale = Vector2(40, 10);
}

void Enemy::Init(const Game& g, const std::string& n)
{
	name = n;

	if (name == "crawler")
	{
		bottomLeftGround = neww Collider(-54, 48, 16, 16);
		bottomRightGround = neww Collider(54, 48, 16, 16);

		bottomLeftGround->CalculateCollider(position, rotation);
		bottomRightGround->CalculateCollider(position, rotation);
	}
	else if (name == "grasshopper")
	{
		CreateCollider(6, 18, 55, 23);
		actionTimer.Start(1);
		physics->jumpSpeed = -1.25f;
	}
	else if (name == "fly")
	{
		CreateCollider(20, 27, 27, 17);
		physics->useGravity = false;
	}
	else if (name == "rolypoly")
	{
		CreateCollider(0, 0, 33, 29);
		//animator->SetBool("directionIsRight", true);
	}
	else if (name == "beehive")
	{
		physics->useGravity = false;
		physics->canBePushed = false;
		health->showHealthBar = false;
		CreateCollider(0, 0, 17, 25);
		layer = DrawingLayer::INVISIBLE;
	}
	else if (name == "bees")
	{
		physics->useGravity = false;
		physics->canBePushed = true;
		health->showHealthBar = true;
		CreateCollider(0, 0, 14, 14);
		actionTimer.Start(1000);
	}
	else if (name == "caterpillar")
	{
		CreateCollider(0, 0, 33, 29);
		//animator->SetBool("directionIsRight", true);
	}
	else if (name == "worm")
	{
		CreateCollider(0, 38, 18, 38);
		animator->SetBool("shouldAppear", true);
		//animator->SetBool("directionIsRight", true);
	}
	else if (name == "termite")
	{
		CreateCollider(0, 0, 60, 28);
		//animator->SetBool("directionIsRight", true);
	}
}

Enemy::~Enemy()
{
	if (bottomLeftGround != nullptr)
		delete_it(bottomLeftGround);

	if (bottomRightGround != nullptr)
		delete_it(bottomRightGround);
}

void Enemy::Update(Game& game)
{
	// TODO: Rather than check the name, initialize variables
	// and then check the individual variables (components)

	health->position = position + Vector2(0, -50);

	bool facePlayer = true;

	if (name == "crawler")
	{
		if (bottomLeftGround != nullptr)
			bottomLeftGround->CalculateCollider(position, rotation);

		if (bottomRightGround != nullptr)
			bottomRightGround->CalculateCollider(position, rotation);

		if (physics->isGrounded)
		{		
			//TODO: Don't move off ledges
			// - Get all entities in quadrant
			// - Compare left or right rectangle against them
			// - If there is not a collision, don't move there

			float distanceToPlayer = std::abs(position.x - game.player->position.x) + 
				std::abs(position.y - game.player->position.y);

			float accel = 0.01f;

			// Only move if the player is nearby
			if (distanceToPlayer < 400.0f)
			{
				if (game.player->position.x > position.x)
					physics->velocity.x = std::min(0.1f, physics->velocity.x + accel);
				else
					physics->velocity.x = std::max(-0.1f, physics->velocity.x - accel);
			}
			else
			{
				if (physics->velocity.x < 0)
					physics->velocity.x = std::min(0.0f, physics->velocity.x + accel);
				else if (physics->velocity.x > 0)
					physics->velocity.x = std::max(0.0f, physics->velocity.x - accel);
			}
		}	
	}
	else if (name == "rhinobeetle")
	{
		animator->SetBool("isGrounded", physics->isGrounded);

		if (actionTimer.HasElapsed())
		{
			physics->Jump(game);
			actionTimer.Start(2000);
		}
		else
		{
			float accel = 0.05f;
			float MAX_SPEED = 2.0f;

			if (physics->velocity.y > 0)
			{
				if (physics->velocity.x < 0)
					physics->velocity.x = std::min(0.0f, physics->velocity.x + accel);
				else if (physics->velocity.x > 0)
					physics->velocity.x = std::max(0.0f, physics->velocity.x - accel);
			}
			else
			{
				if (game.player->position.x > position.x)
					physics->velocity.x = std::min(MAX_SPEED, physics->velocity.x + accel);
				else
					physics->velocity.x = std::max(-MAX_SPEED, physics->velocity.x - accel);
			}

			//std::cout << physics->velocity.x << std::endl;
		}
	}
	else if (name == "grasshopper")
	{
		animator->SetBool("isGrounded", physics->isGrounded);

		if (actionTimer.HasElapsed())
		{			
			physics->Jump(game);
			actionTimer.Start(2000);

			// Don't update this every frame so we stick to the same direction we initially jumped
			animator->SetBool("playerIsToTheRight", game.player->position.x > position.x);
		}
		else
		{
			float accel = 0.05f;
			float MAX_SPEED = 0.5f;

			float distanceToPlayer = std::abs(position.x - game.player->position.x) +
				std::abs(position.y - game.player->position.y);

			if (distanceToPlayer < 400.0f)
			{
				if (physics->isGrounded)
				{
					if (physics->velocity.x < 0)
						physics->velocity.x = std::min(0.0f, physics->velocity.x + accel);
					else if (physics->velocity.x > 0)
						physics->velocity.x = std::max(0.0f, physics->velocity.x - accel);
				}
				else
				{
					if (animator->GetBool("playerIsToTheRight"))
						physics->velocity.x = std::min(MAX_SPEED, physics->velocity.x + accel);
					else
						physics->velocity.x = std::max(-MAX_SPEED, physics->velocity.x - accel);
				}
			}

			//std::cout << physics->velocity.x << std::endl;
		}
	}
	else if (name == "fly")
	{
		float accel = 0.01f;
		float MAX_SPEED = 0.2f;

		float distanceToPlayer = std::abs(position.x - game.player->position.x) +
			std::abs(position.y - game.player->position.y);

		if (distanceToPlayer < 400.0f)
		{
			if (game.player->position.x > position.x)
				physics->velocity.x = std::min(MAX_SPEED, physics->velocity.x + accel);
			else
				physics->velocity.x = std::max(-MAX_SPEED, physics->velocity.x - accel);

			if (game.player->position.y > position.y)
				physics->velocity.y = std::min(MAX_SPEED, physics->velocity.y + accel);
			else
				physics->velocity.y = std::max(-MAX_SPEED, physics->velocity.y - accel);
		}
		else
		{
			if (physics->velocity.x < 0)
				physics->velocity.x = std::min(0.0f, physics->velocity.x + accel);
			else if (physics->velocity.x > 0)
				physics->velocity.x = std::max(0.0f, physics->velocity.x - accel);

			if (physics->velocity.y < 0)
				physics->velocity.y = std::min(0.0f, physics->velocity.y + accel);
			else if (physics->velocity.y > 0)
				physics->velocity.y = std::max(0.0f, physics->velocity.y - accel);
		}
	}
	else if (name == "rolypoly")
	{
		float accel = -0.2f;
		float MAX_SPEED = 0.5f;

		if (animator->currentState->name != "roll")
		{
			accel = 0;
		}
		else if (animator->GetBool("directionIsRight"))
		{
			accel = 0.2f;
		}

		if (physics->isPushed)
		{
			physics->isPushed = false;
			physics->velocity = Vector2(0, 0);
			std::cout << animator->currentState->name << std::endl;
			//isFlippedOver = true;
			//isHurt = false;
			animator->SetBool("isFlippedOver", true);
			animator->SetBool("isHurt", false);
			actionTimer.Start(1500);
		}

		if (animator->GetBool("isFlippedOver"))
		{
			if (actionTimer.HasElapsed())
			{
				//isFlippedOver = false;
				animator->SetBool("isFlippedOver", false);
			}
		}
		else
		{
			// if we're not moving, then start moving
			if (physics->velocity.x == 0 && accel == 0)
			{
				accel = 0.2f;
				physics->velocity.x += accel;
				//directionIsRight = true;
				animator->SetBool("directionIsRight", true);
			}
			else // if we are moving...
			{
				// check for walls and if so, turn around
				if (physics->horizontalCollision)
				{
					//std::cout << "COLLISION! " << physics->previousVelocity.x << std::endl;
					physics->velocity.x = -physics->previousVelocity.x;
					//directionIsRight = (physics->velocity.x > 0);
					animator->SetBool("directionIsRight", (physics->velocity.x > 0));
				}
				else
				{
					if (accel > 0)
						physics->velocity.x = std::min(MAX_SPEED, physics->velocity.x + accel);
					else if (accel < 0)
						physics->velocity.x = std::max(-MAX_SPEED, physics->velocity.x + accel);
				}
			}
		}
	}
	else if (name == "beehive")
	{
		facePlayer = false;
		if (animator->GetBool("isGrounded") && !animator->GetBool("spawnedBees"))
		{
			//TODO: Make this a function we can use elsewhere
			for (int i = 0; i < game.entityTypes["enemy"].size(); i++)
			{
				if (game.entityTypes["enemy"][i] == "bees")
				{
					Enemy* bees = static_cast<Enemy*>(game.SpawnEntity("enemy", position, i));
					bees->Init(game, "bees");
					animator->SetBool("spawnedBees", true);
					trigger = false;
					break;
				}
			}
		}
	}
	else if (name == "bees")
	{
		float accel = 0.02f;
		float MAX_SPEED = 0.5f;

		float distanceToPlayer = std::abs(position.x - game.player->position.x) +
			std::abs(position.y - game.player->position.y);

		if (actionTimer.HasElapsed())
		{
			animator->SetBool("isSwarming", true);

			if (game.player->position.x > position.x)
				physics->velocity.x = std::min(MAX_SPEED, physics->velocity.x + accel);
			else
				physics->velocity.x = std::max(-MAX_SPEED, physics->velocity.x - accel);

			if (game.player->position.y > position.y)
				physics->velocity.y = std::min(MAX_SPEED, physics->velocity.y + accel);
			else
				physics->velocity.y = std::max(-MAX_SPEED, physics->velocity.y - accel);
		}
		else
		{
			if (physics->velocity.x < 0)
				physics->velocity.x = std::min(0.0f, physics->velocity.x + accel);
			else if (physics->velocity.x > 0)
				physics->velocity.x = std::max(0.0f, physics->velocity.x - accel);

			if (physics->velocity.y < 0)
				physics->velocity.y = std::min(0.0f, physics->velocity.y + accel);
			else if (physics->velocity.y > 0)
				physics->velocity.y = std::max(0.0f, physics->velocity.y - accel);
		}
	}

	if (facePlayer)
	{
		if (game.player->position.x > position.x)
			scale.x = -1.0f;
		else
			scale.x = 1.0f;
	}


	physics->Update(game);
}

void Enemy::Render(const Renderer& renderer)
{
	Entity::Render(renderer);

	// TODO: Find a way to get rid of this cast
	MyGUI* myGUI = static_cast<MyGUI*>(renderer.game->gui);
	myGUI->healthComponents.push_back(health);
}

void Enemy::RenderDebug(const Renderer& renderer)
{
	Entity::RenderDebug(renderer);

	if (renderer.game->debugMode && drawDebugRect)
	{
		if (renderer.IsVisible(layer))
		{
			if (bottomLeftGround != nullptr)
			{
				SDL_Rect blRect;
				blRect.x = position.x + bottomLeftGround->offset.x;
				blRect.y = position.y + bottomLeftGround->offset.y;
				blRect.w = bottomLeftGround->bounds->w;
				blRect.h = bottomLeftGround->bounds->h;
				renderer.RenderDebugRect(blRect, Vector2(1, 1));
			}

			if (bottomRightGround != nullptr)
			{
				SDL_Rect brRect;
				brRect.x = position.x + bottomRightGround->offset.x;
				brRect.y = position.y + bottomRightGround->offset.y;
				brRect.w = bottomRightGround->bounds->w;
				brRect.h = bottomRightGround->bounds->h;
				renderer.RenderDebugRect(brRect, Vector2(1, 1));
			}
		}
	}
}

void Enemy::OnTriggerStay(Entity& other, Game& game)
{
	if (other.etype == "debug_missile")
	{
		health->AddCurrentHP(-1);
		if (!health->IsAlive())
		{
			shouldDelete = true;
		}
	}
}

void Enemy::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.etype == "player" && !physics->isPickedUp)
	{
		if (name == "rolypoly")
		{
			if (animator->GetBool("isDead"))
				return;

			// The first time you hit this enemy,
			// it flips the enemy over and exposes its weak spot.
			// For as long as the enemy is flipped over,
			// you can hit it to actually deal damage.
			if (other.name == "debug_missile")
			{
				if (!animator->GetBool("isRecovering"))
				{
					return;
				}
			}
		}
		else if (name == "beehive")
		{
			return;
		}

		other.GetAnimator()->SetBool("isHurt", true);
		other.GetAnimator()->SetState("hurt");
		other.GetAnimator()->Update(other);
		other.GetAnimator()->animationTimer.Start(750);

		const float PUSH_SPEED = 1.0f;

		if (other.position.x < position.x)
		{
			other.physics->velocity.x = -PUSH_SPEED;
		}
		else
		{
			other.physics->velocity.x = PUSH_SPEED;
		}

		other.health->AddCurrentHP(-1);
	}
}

void Enemy::OnTriggerExit(Entity& other, Game& game)
{

}

void Enemy::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["drawOrder"] = std::to_string(drawOrder);
	map["layer"] = std::to_string((int)layer);
	map["impassable"] = std::to_string(impassable);
}

void Enemy::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(map, game);

	subtype = std::stoi(map["subtype"]);
	name = map["name"];

	drawOrder = std::stoi(map["drawOrder"]);
	layer = (DrawingLayer)std::stoi(map["layer"]);
	impassable = std::stoi(map["impassable"]);
}