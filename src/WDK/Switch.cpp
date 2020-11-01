#include "Switch.h"
#include "../ENGINE/Game.h"

Switch::Switch(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 92;
	etype = "switch";
	trigger = true;
	CreateCollider(0, 0, 24, 24);
}

void Switch::OnTriggerStay(Entity& other, Game& game)
{
	if (animator != nullptr)
	{
		animator->SetBool("isPressed", true);
	}
}

void Switch::OnTriggerEnter(Entity& other, Game& game)
{
	if (animator != nullptr)
	{
		if (!animator->GetBool("isPressed"))
		{
			game.soundManager.PlaySound("se/switch.wav", 2);
		}		

		collidingEntities[other.id] = &other;
		animator->SetBool("isPressed", true);	
	}
}

void Switch::OnTriggerExit(Entity& other, Game& game)
{
	if (animator != nullptr)
	{		
		collidingEntities.erase(other.id);
		animator->SetBool("isPressed", (collidingEntities.size() > 0));
	}
}

//TODO: Make sure we save WHAT this switch is actually doing!
void Switch::Save(std::unordered_map<std::string, std::string>& map)
{
	shouldSave = true;
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
}