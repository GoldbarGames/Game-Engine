#include "Enemy.h"
#include "Player.h"
#include "Game.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"

Enemy::Enemy(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::COLLISION;
	drawOrder = 90;
	etype = "enemy";
	trigger = true;

	CreateCollider(0, -4, 54, 32);

	physics = new PhysicsComponent(this);
	physics->useGravity = true;
	physics->canBePushed = true;
	physics->startPosition = pos;
	physics->standAboveGround = true;

	health = new HealthComponent(4);
	health->showHealthBar = true;
	health->initialHealthBarScale = Vector2(40, 10);
}

void Enemy::Init(const std::string& n)
{
	name = n;

	if (name == "crawler")
	{
		bottomLeftGround = new Collider(-54, 48, 16, 16);
		bottomRightGround = new Collider(54, 48, 16, 16);

		bottomLeftGround->CalculateCollider(position);
		bottomRightGround->CalculateCollider(position);
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
			bottomLeftGround->CalculateCollider(position);

		if (bottomRightGround != nullptr)
			bottomRightGround->CalculateCollider(position);

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
			playerIsToTheRight = game.player->position.x > position.x;
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
					if (playerIsToTheRight)
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
					bees->Init("bees");
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

	renderer.game->gui.healthComponents.push_back(health);

	if (renderer.game->debugMode && drawDebugRect)
	{
		//TODO: Refactor this? It seems like this is not very efficient
		if (debugSprite == nullptr)
			debugSprite = new Sprite(renderer.debugSprite->texture, renderer.debugSprite->shader);

		if (renderer.IsVisible(layer))
		{
			//TODO: Make this a function inside the renderer

			float rWidth = debugSprite->texture->GetWidth();
			float rHeight = debugSprite->texture->GetHeight();

			if (bottomLeftGround != nullptr)
			{
				// draw collider
				float targetWidth = bottomLeftGround->bounds->w;
				float targetHeight = bottomLeftGround->bounds->h;

				debugSprite->color = { 255, 255, 255, 255 };
				//debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));

				Vector2 colliderPosition = Vector2(position.x + bottomLeftGround->offset.x, 
					position.y + bottomLeftGround->offset.y);
				debugSprite->Render(colliderPosition, renderer);
			}

			if (bottomRightGround != nullptr)
			{
				// draw collider
				float targetWidth = bottomRightGround->bounds->w;
				float targetHeight = bottomRightGround->bounds->h;

				debugSprite->color = { 255, 255, 255, 255 };
				//debugSprite->pivot = GetSprite()->pivot;
				debugSprite->SetScale(Vector2(targetWidth / rWidth, targetHeight / rHeight));

				Vector2 colliderPosition = Vector2(position.x + bottomRightGround->offset.x,
					position.y + bottomRightGround->offset.y);
				debugSprite->Render(colliderPosition, renderer);
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
	if (other.etype == "player")
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

void Enemy::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << physics->startPosition.x <<
		" " << physics->startPosition.y << " " << name << " " << subtype << " "
		<< drawOrder << " " << (int)layer << " " << impassable << std::endl;
}