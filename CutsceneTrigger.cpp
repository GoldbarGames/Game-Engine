#include "CutsceneTrigger.h"
#include "Player.h"
#include "Game.h"

CutsceneTrigger::CutsceneTrigger(std::string label, Vector2 pos, float w, float h) : Entity(pos)
{
	cutsceneLabel = label;
	triggerRect.x = pos.x;
	triggerRect.y = pos.y;
	triggerRect.w = w;
	triggerRect.h = h;
}


CutsceneTrigger::~CutsceneTrigger()
{
	
}

const SDL_Rect* CutsceneTrigger::GetBounds()
{
	return &triggerRect;
}

void CutsceneTrigger::OnTriggerStay(Entity* other)
{

}

void CutsceneTrigger::OnTriggerEnter(Entity* other)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		if (player->game != nullptr)
			player->game->cutscene->PlayCutscene(cutsceneLabel);
	}
}

void CutsceneTrigger::OnTriggerExit(Entity* other)
{

}
