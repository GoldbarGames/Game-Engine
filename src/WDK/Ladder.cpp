#include "Ladder.h"
#include "Player.h"
#include "../ENGINE/globals.h"
#include "../ENGINE/Renderer.h"
#include "../ENGINE/Game.h"
#include "../ENGINE/Editor.h"
#include "../ENGINE/Animator.h"
#include "../ENGINE/Sprite.h"
#include "MyEditorHelper.h"

Ladder::Ladder(const Vector2& pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::COLLISION;
	drawOrder = 90;
	trigger = true;
	CreateCollider(0, 0, TILE_SIZE, TILE_SIZE);
	shouldSave = true;
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

	if (game.editor->helper != nullptr)
	{
		MyEditorHelper* helper = static_cast<MyEditorHelper*>(game.editor->helper);

		if (helper->loadListLadderGroups.count(position.x) == 0)
		{
			helper->loadListLadderGroups[position.x] = std::vector<Ladder*>();
		}
		helper->loadListLadderGroups[position.x].push_back(this);
	}


}