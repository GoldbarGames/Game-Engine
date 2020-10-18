#include "MyEntityFactory.h"
#include "Door.h"
#include "Ladder.h"
#include "Platform.h"
#include "Player.h"
#include "NPC.h"
#include "Block.h"
#include "Shroom.h"
#include "Missile.h"
#include "Enemy.h"
#include "Switch.h"
#include "Collectible.h"
#include "Checkpoint.h"
#include "Tree.h"
#include "Decoration.h"

MyEntityFactory::MyEntityFactory() : EntityFactory()
{
    //Register("tile", &Tile::Create);
    Register("door", &Door::Create);
    Register("ladder", &Ladder::Create);
    Register("platform", &Platform::Create);
    Register("player", &Player::Create);
    Register("npc", &NPC::Create);
    Register("block", &Block::Create);
    Register("collectible", &Collectible::Create);
    Register("shroom", &Shroom::Create);
    Register("missile", &Missile::Create);
    Register("enemy", &Enemy::Create);
    Register("switch", &Switch::Create);
    Register("checkpoint", &Checkpoint::Create);
    Register("tree", &Tree::Create);
    Register("decoration", &Decoration::Create);
}