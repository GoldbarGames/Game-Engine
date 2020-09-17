#include "Ladder.h"
#include "Player.h"
#include "globals.h"
#include "Renderer.h"
#include "Game.h"

Ladder::Ladder(const Vector2& pos) : Entity(pos)
{
	etype = "ladder";
	layer = DrawingLayer::OBJECT;
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

void Ladder::Save(std::ostringstream& level)
{
	level << std::to_string(id) 
		<< " " << etype 
		<< " " << GetPosition().x 
		<< " " << GetPosition().y 
		<< " " << GetAnimator()->currentState->name
		<< " " << subtype 
		<< "" << std::endl;
}

void Ladder::Load(int& index, const std::vector<std::string>& tokens,
	std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(index, tokens, map, game);

	subtype = std::stoi(tokens[index++]);
	std::string ladderState = tokens[index++];
	GetAnimator()->SetState(ladderState.c_str());

	if (game.editor->loadListLadderGroups.count(position.x) == 0)
	{
		game.editor->loadListLadderGroups[position.x] = std::vector<Ladder*>();
	}
	game.editor->loadListLadderGroups[position.x].push_back(this);
}