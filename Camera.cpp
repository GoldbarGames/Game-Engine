#include "Camera.h"
#include <SDL_scancode.h>

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch, GLfloat startMovementSpeed, GLfloat startTurnSpeed)
{
	position = startPosition;
	worldUp = startUp;
	yaw = startYaw;
	pitch = startPitch;
	front = glm::vec3(0.0f, 0.0f, -1.0f);

	movementSpeed = startMovementSpeed;
	turnSpeed = startTurnSpeed;

	Update();
}

Camera::Camera()
{

}

Camera::~Camera()
{

}

glm::mat4 Camera::CalculateViewMatrix()
{
	return glm::lookAt(position, position + front, up);
}

void Camera::MouseControl(GLfloat xChange, GLfloat yChange)
{
	xChange *= turnSpeed;
	yChange *= turnSpeed;

	yaw += xChange;
	pitch += yChange;

	if (pitch > 89.0f)
	{
		pitch = 89.0f;
	}

	if (pitch < -89.0f)
	{
		pitch = -89.0f;
	}

	Update();
}

void Camera::KeyControl(const Uint8* input, GLfloat dt)
{
	GLfloat velocity = movementSpeed * dt;

	if (input[SDL_SCANCODE_K])
	{
		position += up * velocity;
	}

	if (input[SDL_SCANCODE_I])
	{
		position -= up * velocity;
	}

	if (input[SDL_SCANCODE_J])
	{
		position -= right * velocity;
	}

	if (input[SDL_SCANCODE_L])
	{
		position += right * velocity;
	}
}

void Camera::Update()
{
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	front = glm::normalize(front);

	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}