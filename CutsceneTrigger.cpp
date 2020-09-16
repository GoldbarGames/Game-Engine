#include "CutsceneTrigger.h"
#include "Player.h"
#include "Game.h"

CutsceneTrigger::CutsceneTrigger(Vector2 pos) : Entity(pos)
{

}

CutsceneTrigger::~CutsceneTrigger()
{
	
}

void CutsceneTrigger::Init(const std::string& n)
{
	name = n;
}

const SDL_Rect* CutsceneTrigger::GetBounds()
{
	return &triggerRect;
}

void CutsceneTrigger::OnTriggerStay(Entity& other, Game& game)
{

}

void CutsceneTrigger::OnTriggerEnter(Entity& other, Game& game)
{
	if (other.etype == "player")
	{
		game.cutscene->PlayCutscene(cutsceneLabel.c_str());
	}
}

void CutsceneTrigger::OnTriggerExit(Entity& other, Game& game)
{

}

void CutsceneTrigger::Save(std::ostringstream& level)
{

}

void CutsceneTrigger::Load(int& index, const std::vector<std::string>& tokens,
	std::unordered_map<std::string, std::string>& map, Game& game)
{
	Entity::Load(index, tokens, map, game);

	cutsceneLabel = tokens[index++];
	triggerRect.x = (int)position.x;
	triggerRect.y = (int)position.y;
	triggerRect.w = (int)std::stoi(tokens[index++]);
	triggerRect.h = (int)std::stoi(tokens[index++]);
}