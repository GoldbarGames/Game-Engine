#include "Collider.h"

Collider::Collider(float x, float y, float w, float h)
{
	CreateCollider(x, y, w, h);
};

Collider::~Collider()
{
	if (bounds != nullptr)
		delete_it(bounds);
}

void Collider::CreateCollider(float x, float y, float w, float h)
{
	offset.x = x;
	offset.y = y;
	scale.x = w;
	scale.y = h;

	if (bounds == nullptr)
		bounds = new SDL_Rect();

	//CalculateCollider();
}

void Collider::CalculateCollider(const Vector2& position)
{
	bounds->w = scale.x;
	bounds->h = scale.y;
	bounds->x = (int)(position.x + offset.x);
	bounds->y = (int)(position.y + offset.y);
}