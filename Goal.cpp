#include "Goal.h"
#include "Player.h"
#include "Game.h"

Goal::Goal(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "goal";
	trigger = true;
}

Goal::~Goal()
{

}

void Goal::Update(Game& game)
{
	isOpen = game.bugsRemaining <= 0;
	animator->SetBool("opened", isOpen);
	Entity::Update(game);
}

void Goal::OnTriggerStay(Entity* other, Game& game)
{

}

void Goal::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentGoal = this;
	}
}

void Goal::OnTriggerExit(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		player->currentGoal = nullptr;
	}
}

void Goal::GetProperties(Renderer * renderer, FontInfo* font, std::vector<Property*>& properties)
{
	Entity::GetProperties(font, properties);

	properties.emplace_back(new Property(new Text(font, "Next Level: " + nextLevelName)) );
}

void Goal::SetProperty(const std::string& key, const std::string& newValue)
{
	// Based on the key, change its value
	if (key == "Next Level")
	{
		nextLevelName = newValue;
	}
}

void Goal::Save(std::ostringstream& level)
{
	level << std::to_string(id) << " " << etype << " " << GetPosition().x <<
		" " << GetPosition().y << " " << spriteIndex << std::endl;
}