#include "MyEntity.h"
#include "../ENGINE/PhysicsComponent.h"
#include "../ENGINE/HealthComponent.h"

MyEntity::MyEntity(const Vector2& pos) : Entity(pos)
{

}

MyEntity::MyEntity(const Vector2& pos, Sprite* sprite) : Entity(pos, sprite)
{

}

MyEntity::~MyEntity()
{
	if (physics != nullptr)
		delete_it(physics);
	if (health != nullptr)
		delete_it(health);
}

void MyEntity::Update(Game& game)
{
	Entity::Update(game);
	if (physics != nullptr)
		physics->Update(game);
}


void MyEntity::OnTriggerStay(MyEntity& other, Game& game)
{

}

void MyEntity::OnTriggerEnter(MyEntity& other, Game& game)
{

}

void MyEntity::OnTriggerExit(MyEntity& other, Game& game)
{

}