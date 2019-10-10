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

void Goal::GetProperties(Renderer * renderer, TTF_Font * font, std::vector<Text*>& properties)
{
	Entity::DeleteProperties(properties);

	properties.emplace_back(new Text(renderer, font, "Next Level: " + nextLevelName));
}

void Goal::SetProperty(std::string prop, std::string newValue)
{
	// 1. Split the string into two (key and value)
	std::string key = "";

	int index = 0;
	while (prop[index] != ':')
	{
		key += prop[index];
		index++;
	}

	// 2. Based on the key, change its value
	if (key == "Next Level")
	{
		nextLevelName = newValue;
	}
}