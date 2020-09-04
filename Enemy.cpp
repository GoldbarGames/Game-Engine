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
		" " << physics->startPosition.y << " " << name << " " << spriteIndex << " "
		<< drawOrder << " " << (int)layer << " " << impassable << std::endl;
}