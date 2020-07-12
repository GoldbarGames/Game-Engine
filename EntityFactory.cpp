#include "EntityFactory.h"
#include "Door.h"
#include "Ladder.h"
#include "Platform.h"
#include "Player.h"

EntityFactory::EntityFactory()
{
    Register("entity", &Entity::Create);

    Register("door", &Door::Create);
    Register("ladder", &Ladder::Create);
    Register("platform", &Platform::Create);
    Register("player", &Player::Create);    
}

void EntityFactory::Register(const std::string& entityName, CreateEntity pfnCreate)
{
    entities[entityName] = pfnCreate;
}

Entity* EntityFactory::Create(const std::string& entityName, const Vector2& position)
{   
    if (entities.count(entityName) == 1)
        return entities[entityName](position);
    return entities["entity"](position);
}