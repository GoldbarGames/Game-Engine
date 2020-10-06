#include "Collectible.h"
#include <sstream>
#include "Player.h"
#include "Game.h"
#include "PhysicsComponent.h"
#include "HealthComponent.h"
#include "Text.h"
#include "Missile.h"


Collectible::Collectible(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::COLLISION;
	drawOrder = 90;
	etype = "collectible";
	trigger = true;
}

Collectible::~Collectible()
{

}

void Collectible::Update(Game& game)
{
	Entity::Update(game);
}

void Collectible::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Collectible::Init(const std::string& n)
{
	name = n;

	if (name == "ether")
	{
		CreateCollider(0, -4, 54, 32);
	}
	else if (name == "bug")
	{
		CreateCollider(0, -4, 15, 18);

		physics = neww PhysicsComponent(this);
		physics->useGravity = false;
		startPosition = position;
	}
	else if (name == "heart")
	{
		CreateCollider(0, 0, 32, 32);
	}
}

void Collectible::OnTriggerStay(Entity& other, Game& game)
{

}

void Collectible::OnTriggerEnter(Entity& other, Game& game)
{
	if (name == "ether")
	{
		if (other.etype == "player")
		{
			Player* player = static_cast<Player*>(&other);
			game.currentEther++;
			game.etherText->SetText("Ether: " + std::to_string(game.currentEther));
			shouldDelete = true;
		}
	}
	else if (name == "bug")
	{
		if (other.etype == "debug_missile")
		{
			Missile* missile = dynamic_cast<Missile*>(&other);

			if (!missile->GetAnimator()->GetBool("destroyed"))
			{
				shouldDelete = true;
				missile->Destroy();
				game.bugsRemaining--;
				game.bugText->SetText("Bugs Remaining: " + std::to_string(game.bugsRemaining));
			}
		}
	}
	else if (name == "heart")
	{
		if (other.etype == "player")
		{
			Player* player = static_cast<Player*>(&other);
			player->health->AddCurrentHP(1);
			shouldDelete = true;
		}
	}
}

void Collectible::OnTriggerExit(Entity& other, Game& game)
{

}
