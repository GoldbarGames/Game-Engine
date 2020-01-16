#include "Camera.h"
#include <SDL_scancode.h>

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, 
	GLfloat startYaw, GLfloat startPitch, 
	GLfloat startMovementSpeed, GLfloat startTurnSpeed,
	GLfloat startZoom)
{
	position = startPosition;
	worldUp = startUp;
	yaw = startYaw;
	pitch = startPitch;
	front = glm::vec3(0.0f, 0.0f, -1.0f);

	movementSpeed = startMovementSpeed;
	turnSpeed = startTurnSpeed;

	projection = glm::mat4(1.0f);
	orthoZoom = startZoom;

	float screenWidth = 1280.0f;
	float screenHeight = 720.0f;

	Zoom(0.0f, screenWidth, screenHeight);

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
	return glm::lookAt(position, position - front, up);
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
		position += right * velocity;
	}

	if (input[SDL_SCANCODE_L])
	{
		position -= right * velocity;
	}

	if (input[SDL_SCANCODE_N])
	{
		Zoom(-0.025f, 1280.0f, 720.0f);
	}

	if (input[SDL_SCANCODE_M])
	{
		Zoom(0.025f, 1280.0f, 720.0f);
	}
}

void Camera::Zoom(float amount, float screenWidth, float screenHeight)
{
	orthoZoom += amount;

	if (useOrthoCamera)
	{
		projection = glm::ortho(0.0f, (GLfloat)screenWidth * orthoZoom,
			(GLfloat)screenHeight * orthoZoom, 0.0f, -1.0f, 10.0f);
	}
	else
	{
		GLfloat aspectRatio = (GLfloat)screenWidth / (GLfloat)screenHeight;
		projection = glm::perspective(45.0f, aspectRatio, 0.1f, 100.0f);
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