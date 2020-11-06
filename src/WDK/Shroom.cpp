#include "Shroom.h"
#include "../ENGINE/Game.h"
#include "PhysicsComponent.h"
#include "../ENGINE/SoundManager.h"
#include "../ENGINE/Sprite.h"

Shroom::Shroom(const Vector2& pos) : MyEntity(pos)
{
	etype = "shroom";
	CreateCollider(0, -15, 24, 32);
	layer = DrawingLayer::COLLISION;
	drawOrder = 10;

	impassable = false;
	trigger = true;

	physics = neww PhysicsComponent(this);
	physics->canBePushed = true;
	physics->standAboveGround = true;
	physics->respawnOnDeath = true;
	physics->mass = 10;
}

Shroom::~Shroom()
{

}

void Shroom::Init(const Game& g, const std::string& n)
{
	name = n;

	if (name == "shroom")
	{
		physics->canBePushed = false;
		physics->useGravity = false;
	}
	else
	{
		physics->canBePushed = true;
		physics->useGravity = true;
		physics->canBePickedUp = true;
	}
}

void Shroom::Update(Game& game)
{
	if (animator->GetBool("isBouncing") && currentSprite.HasAnimationElapsed())
	{
		animator->SetBool("isBouncing", false);
	}

	Entity::Update(game);
}


void Shroom::OnTriggerEnter(MyEntity& other, Game& game)
{
	if (other.physics != nullptr && other.physics->canBePushed)
	{
		//TODO: This doesn't work if we are already moving in that direction,
		// probably because it gets set equal to whatever at a later point

		switch ((int)rotation.z)
		{
		case 0:
			other.physics->velocity.y = -1.5f;
			break;
		case 90:
			other.physics->velocity.x = -1.5f;
			break;
		case 180:
			other.physics->velocity.y = 1.5f;
			break;
		case 270:
			other.physics->velocity.x = 1.5f;
			break;
		default:
			other.physics->velocity.y = -1.5f;
			break;
		}

		game.soundManager.PlaySound("se/Jump.wav", 0);
		animator->SetBool("isBouncing", true);
		animator->Update(*this);
		animator->DoState(*this);
	}
}

void Shroom::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
}