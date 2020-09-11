#include "CutsceneTrigger.h"
#include "Player.h"
#include "Game.h"

CutsceneTrigger::CutsceneTrigger(std::string label, Vector2 pos, float w, float h) : Entity(pos)
{
	cutsceneLabel = label;
	triggerRect.x = (int)pos.x;
	triggerRect.y = (int)pos.y;
	triggerRect.w = (int)w;
	triggerRect.h = (int)h;
}

CutsceneTrigger::~CutsceneTrigger()
{
	
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
