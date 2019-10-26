#include "Ether.h"
#include "Player.h"
#include "Game.h"

Ether::Ether(Vector2 pos) : Entity(pos)
{
	layer = DrawingLayer::OBJECT;
	drawOrder = 90;
	etype = "ether";
	trigger = true;
}

Ether::~Ether()
{

}

void Ether::OnTriggerStay(Entity* other, Game& game)
{

}

void Ether::OnTriggerEnter(Entity* other, Game& game)
{
	if (other->etype == "player")
	{
		Player* player = static_cast<Player*>(other);
		game.currentEther++;
		game.etherText->SetText("Ether: " + std::to_string(game.currentEther));
		shouldDelete = true;
	}
}

void Ether::OnTriggerExit(Entity* other, Game& game)
{

}

void Ether::Save(std::ostringstream& level)
{
	int SCALE = Renderer::GetScale();

	level << std::to_string(id) << " " << etype << " " << (GetPosition().x / SCALE) <<
		" " << (GetPosition().y / SCALE) << " " << spriteIndex << std::endl;
}