#include "leak_check.h"
#include "Collider.h"

Collider::Collider(float x, float y, float w, float h)
{
	CreateCollider(x, y, w, h);
};

Collider::~Collider()
{

}

void Collider::CreateCollider(float x, float y, float w, float h)
{
	offset.x = x;
	offset.y = y;
	scale.x = w;
	scale.y = h;

	CalculateCollider(glm::vec3(x, y, 0), glm::vec3(0,0,0));
}

void Collider::CalculateCollider(const glm::vec3& position, const glm::vec3& rotation)
{
	bool isRotated = (rotation.z == 90 || rotation.z == 270);
	bounds.w = isRotated ? scale.y : scale.x;
	bounds.h = isRotated ? scale.x : scale.y;
	bounds.x = (int)(position.x + offset.x);
	bounds.y = (int)(position.y + offset.y);
}