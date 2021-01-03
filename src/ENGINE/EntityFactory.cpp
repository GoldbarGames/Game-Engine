#include "EntityFactory.h"
#include "Entity.h"
#include "CameraBounds.h"

EntityFactory::EntityFactory()
{
    Register("entity", &Entity::Create);
    Register("cameraBounds", &CameraBounds::Create);
}

void EntityFactory::Register(const std::string& entityName, CreateEntity pfnCreate) const
{
    entities[entityName] = pfnCreate;
}

Entity* EntityFactory::Create(const std::string& entityName, const glm::vec3& position) const
{   
    if (entities.count(entityName) == 1)
        return entities[entityName](position);
    return nullptr;
}