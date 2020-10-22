#include "leak_check.h"
#include "Collider.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"

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
		bounds = neww SDL_Rect();
}

void Collider::CalculateCollider(const Vector2& position, const glm::vec3& rotation)
{
	bounds->x = position.x + (Camera::MULTIPLIER * offset.x);
	bounds->y = position.y + (Camera::MULTIPLIER * offset.y);
	bounds->w = scale.x;
	bounds->h = scale.y;
	
	return;


	if (rotation.z != 0.0f)
		int test = 0;
	else
		int test = 0;

	/*
	mat4 result = glm::translate(-pivot) *
              glm::scale(..) *
              glm::rotate(..) *
              glm::translate(pivot) *
              glm::translate(..);
	*/

	//Vector2 pivot = Vector2(offset.x * Camera::MULTIPLIER, offset.y * Camera::MULTIPLIER); // Vector2(0, 0);
	Vector2 pivot = offset;
	const float toRadians = 3.14159265f / 180.0f;

	glm::mat4 modelStartPos = glm::mat4(1.0f);
	modelStartPos = glm::translate(modelStartPos, glm::vec3(-pivot.x, -pivot.y, 0.0f));
	//modelStartPos = glm::scale(modelStartPos, glm::vec3(-1 * scale.x, scale.y, 1.0f));
	modelStartPos = glm::rotate(modelStartPos, rotation.z * toRadians, glm::vec3(position.x, position.y, 1.0));
	modelStartPos = glm::translate(modelStartPos, glm::vec3(pivot.x, pivot.y, 0.0f));
	modelStartPos = glm::translate(modelStartPos, glm::vec3(position.x, position.y, 0.0f));

	//glm::mat4 scaleModel = glm::mat4(1.0f);
	//scaleModel = glm::scale(scaleModel, glm::vec3(-1 * scale.x, scale.y, 1.0f));

	glm::mat4 modelEndPos = glm::mat4(1.0f);
	modelEndPos = glm::translate(modelEndPos, glm::vec3(-pivot.x, -pivot.y, 0.0f));
	//modelEndPos = glm::scale(modelStartPos, glm::vec3(-1 * scale.x, scale.y, 1.0f));
	modelEndPos = glm::rotate(modelEndPos, rotation.z * toRadians, glm::vec3(position.x, position.y, 0.0f));
	modelEndPos = glm::translate(modelEndPos, glm::vec3(pivot.x, pivot.y, 0.0f));
	modelEndPos = glm::translate(modelEndPos, glm::vec3(position.x + scale.x, position.y + scale.y, 0.0f));

	glm::vec4 vecStartPos = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	vecStartPos = glm::vec4(modelStartPos * vecStartPos);

	glm::vec4 vecEndPos = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
	vecEndPos = glm::vec4(modelEndPos * vecEndPos);

	bounds->x = vecStartPos.x - 1; // / (float)Camera::MULTIPLIER;
	bounds->y = vecStartPos.y - 1; // / (float)Camera::MULTIPLIER;

	bounds->w = std::abs(vecEndPos.x - vecStartPos.x);

	if (rotation.z == 90 || rotation.z == 270)
	{
		bounds->h = std::abs(vecEndPos.z - vecStartPos.z);
	}
	else
	{
		bounds->h = std::abs(vecEndPos.y - vecStartPos.y);
	}
	
	int test = 0;
}



/*
*
* 	Vector2 newPosition = position;
\
bool isRotated = (rotation.z == 90 || rotation.z == 270);
bounds->w = isRotated ? scale.y : scale.x;
bounds->h = isRotated ? scale.x : scale.y;

if (rotation.z == 90)
{
	bounds->x = (int)(position.x) + (offset.y / 2);
	bounds->y = (int)(position.y) + (scale.y);
}
else if (rotation.z == 180)
{
	bounds->x = (int)(position.x);
	bounds->y = (int)(position.y);
}
else if (rotation.z == 270)
{
	bounds->x = (int)(position.x) - (offset.y / 2);
	bounds->y = (int)(position.y) + (scale.y);
}
else
{
	bounds->x = (int)(position.x + offset.x);
	bounds->y = (int)(position.y + offset.y);
}
*/