#pragma once
#include <string>
#include <unordered_map>

class Entity;
class Vector2;

typedef Entity* (__stdcall* CreateEntity)(const Vector2& pos);

class EntityFactory
{
private:
    EntityFactory();
    EntityFactory(const EntityFactory&) { }
    EntityFactory& operator=(const EntityFactory&) { return *this; }

    std::unordered_map<std::string, CreateEntity> entities;
public:
    ~EntityFactory() { entities.clear(); }

    static EntityFactory* Get()
    {
        static EntityFactory instance;
        return &instance;
    }

    void Register(const std::string& entityName, CreateEntity pfnCreate);
    Entity* Create(const std::string& entityName, const Vector2& position);
};

