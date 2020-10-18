#pragma once
#include <string>
#include <unordered_map>

class Entity;
class Vector2;

typedef Entity* (__stdcall* CreateEntity)(const Vector2& pos);

class __declspec(dllexport) EntityFactory
{
private:    
    mutable std::unordered_map<std::string, CreateEntity> entities;
protected:
    EntityFactory();
    EntityFactory(const EntityFactory&) { }
    EntityFactory& operator=(const EntityFactory&) { return *this; }
    void Register(const std::string& entityName, CreateEntity pfnCreate) const;
public:
    ~EntityFactory() { entities.clear(); }

    static EntityFactory* Get()
    {
        static EntityFactory instance;
        return &instance;
    }

    Entity* Create(const std::string& entityName, const Vector2& position) const;
};

