#include "Ladder.h"
#include "Player.h"
#include "globals.h"
#include "Renderer.h"
#include "Game.h"
#include "Editor.h"
#include "Animator.h"
#include "Sprite.h"

Ladder::Ladder(const Vector2& pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::COLLISION;
	drawOrder = 90;
	trigger = true;
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
}

Ladder::~Ladder()
{
	
}

void Ladder::OnTriggerStay(Entity& other, Game& game)
{

}

void Ladder::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		player->currentLadder = this;
	}
}

void Ladder::OnTriggerExit(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		Player* player = static_cast<Player*>(&other);
		if (player->currentLadder == this)
		{			
			player->GetAnimator()->SetBool("onLadder", false);
			player->currentLadder = nullptr;
		}		
	}
}

void Ladder::Render(const Renderer& renderer)
{
	Entity::Render(renderer);
}

void Ladder::Save(std::unordered_map<std::string, std::string>& map)
{
	Entity::Save(map);

	map["subtype"] = std::to_string(subtype);
	map["ladderState"] = GetAnimator()->currentState->name;
}

void Ladder::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	shouldSave = true;
	Entity::Load(map, game);

	subtype = std::stoi(map["subtype"]);
	std::string ladderState = map["ladderState"];
	GetAnimator()->SetState(ladderState.c_str());

	if (game.editor->loadListLadderGroups.count(position.x) == 0)
	{
		game.editor->loadListLadderGroups[position.x] = std::vector<Ladder*>();
	}
	game.editor->loadListLadderGroups[position.x].push_back(this);
}