#include "Camera.h"
#include "Entity.h"
#include <SDL_scancode.h>
#include "globals.h"
#include "Game.h"

float Camera::MULTIPLIER = 2.0f;

Camera::Camera(glm::vec3 startPosition, glm::vec3 startUp, 
	float startYaw, float startPitch,
	float startMovementSpeed, float startTurnSpeed,
	float startZoom, float width, float height, bool useOrtho)
{
	useOrthoCamera = useOrtho;
	
	position = startPosition;

	if (useOrthoCamera)
		position.z = 0;

	worldUp = startUp;
	yaw = startYaw;
	pitch = startPitch;
	front = glm::vec3(0.0f, 0.0f, -1.0f);

	movementSpeed = startMovementSpeed;
	turnSpeed = startTurnSpeed;

	angle = -45.0f;

	startingZoom = startZoom;
	orthoZoom = startZoom;

	projection = glm::mat4(1.0f);

	// This is what scales the gui-scaled images when we change screen resolutions
	startScreenWidth = width;
	startScreenHeight = height;
	guiProjection = glm::ortho(0.0f, startScreenWidth * MULTIPLIER, startScreenHeight * MULTIPLIER, 0.0f, -1.0f, 10.0f);

	Zoom(0.0f, width, height);

	Update();
}

Camera::Camera()
{
	
}

Camera::~Camera()
{

}

void Camera::ResetCamera()
{
	position = glm::vec3(0, 0, 0);
	angle = 0.0f;
	ResetProjection();
}

void Camera::SwitchTarget(const Entity& newTarget)
{
	switchingTarget = true;
	target = &newTarget;
	nextPosition = glm::vec3(target->GetPosition().x - (startScreenWidth * 0.5f),
		target->GetPosition().y - (startScreenHeight * 0.5f), position.z);
}

void Camera::FollowTarget(const Game& game)
{
	static bool finishedLerp = false;

	if (target != nullptr)
	{
		if (switchingTarget) // gradually move camera to the new target
		{
			bool isX = (int)position.x == (int)nextPosition.x;
			bool isY = (int)position.y == (int)nextPosition.y;

			if (isX && isY)
			{
				position = nextPosition;
				switchingTarget = false;
			}
			else
			{
				if (startTime == 0)
				{
					startTime = game.timer.GetTicks();
					endTime = startTime + 3000;
				}

				finishedLerp = LerpVector3(finishedLerp, position, position, nextPosition, game.timer.GetTicks(), startTime, endTime);
			}
		}
		else // gradually move camera to current target only if outside bounds
		{
			// The position is the top left corner, so to get the target in the center
			// of the screen, we need to offset it by half the screen's width and height

			glm::vec3 targetCenter = glm::vec3(target->GetPosition().x - (startScreenWidth * 0.5f),
				target->GetPosition().y - (startScreenHeight * 0.5f),
				position.z);

			nextPosition = glm::vec3(targetCenter.x, targetCenter.y, position.z);

			startTime = game.timer.GetTicks();

			if (targetCenter.x > position.x + (startScreenWidth * 0.5f))
			{
				finishedLerp = false;
				nextPosition = glm::vec3(targetCenter.x + (startScreenWidth * 0.5f), position.y, position.z);
				finishedLerp = LerpVector3(finishedLerp, position, position, nextPosition, startTime, startTime, startTime);
			}
			else if (targetCenter.x < position.x - (startScreenWidth * 0.5f))
			{
				finishedLerp = false;
				nextPosition = glm::vec3(targetCenter.x - (startScreenWidth * 0.5f), position.y, position.z);
				finishedLerp = LerpVector3(finishedLerp, position, position, nextPosition, startTime, startTime, startTime);
			}
			else if (targetCenter.y > position.y + (startScreenHeight * 0.5f))
			{
				finishedLerp = false;
				nextPosition = glm::vec3(position.x, targetCenter.y + (startScreenHeight * 0.5f), position.z);
				finishedLerp = LerpVector3(finishedLerp, position, position, nextPosition, startTime, startTime, startTime);
			}
			else if (targetCenter.y < position.y - (startScreenHeight * 0.5f))
			{
				finishedLerp = false;
				nextPosition = glm::vec3(position.x, targetCenter.y - (startScreenHeight * 0.5f), position.z);
				finishedLerp = LerpVector3(finishedLerp, position, position, nextPosition, startTime, startTime, startTime);
			}
			else
			{
				//position = nextPosition;
				finishedLerp = LerpVector3(finishedLerp, position, position, nextPosition, startTime, startTime, startTime);
			}


		}



	}
}

glm::mat4 Camera::CalculateViewMatrix() const
{
	return glm::lookAt(position, position - front, up);
}

void Camera::MouseControl(float xChange, float yChange)
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

void Camera::KeyControl(const Uint8* input, const float& dt, 
	const float& screenWidth, const float& screenHeight)
{
	GLfloat velocity = movementSpeed * dt * orthoZoom;

	// 2D Movement

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

	// 3D Movement

	if (!useOrthoCamera)
	{
		if (input[SDL_SCANCODE_O])
		{
			position += front * velocity;
		}

		if (input[SDL_SCANCODE_P])
		{
			position -= front * velocity;
		}
	}

	// 3D rotation

	if (!useOrthoCamera)
	{
		if (input[SDL_SCANCODE_N])
		{
			angle += 0.01f;
			Zoom(0.0f, screenWidth, screenHeight);
		}
		if (input[SDL_SCANCODE_M])
		{
			angle -= 0.01f;
			Zoom(0.0f, screenWidth, screenHeight);
		}

		if (input[SDL_SCANCODE_U])
		{
			if (!shouldUpdate)
				return;

			//yaw += xChange;
			pitch += 1.0f;

			if (pitch > 89.0f)
				pitch = 89.0f;

			if (pitch < -89.0f)
				pitch = -89.0f;

			Update();
		}

		if (input[SDL_SCANCODE_Y])
		{
			if (!shouldUpdate)
				return;

			//yaw += xChange;
			pitch -= 1.0f;

			if (pitch > 89.0f)
				pitch = 89.0f;

			if (pitch < -89.0f)
				pitch = -89.0f;

			Update();
		}
	}
	else // 2D Zoom
	{
		//if (!editMode)
		//{
		//	
		//}

		if (input[SDL_SCANCODE_N])
		{
			Zoom(-0.25f, screenWidth, screenHeight);
		}

		if (input[SDL_SCANCODE_M])
		{
			Zoom(0.25f, screenWidth, screenHeight);
		}
	}
}

void Camera::ResetProjection()
{
	if (useOrthoCamera)
	{
		float zoomX = (800 * startingZoom);
		float zoomY = (600 * startingZoom);
		projection = glm::ortho(0.0f, zoomX, zoomY, 0.0f, -1.0f, 10.0f);
	}
	else
	{
		float aspectRatio = 1280.0f / 720.0f;
		projection = glm::perspective(angle, -aspectRatio, 0.001f, 10000.0f);
	}
}

void Camera::Zoom(float amount, float screenWidth, float screenHeight)
{
	orthoZoom += amount;

	// For this to work correctly you need to use the original width/height
	// which in our case is 1280 x 720

	if (useOrthoCamera)
	{
		float zoomX = (screenWidth * orthoZoom);
		float zoomY = (screenHeight * orthoZoom);
		projection = glm::ortho(0.0f, zoomX, zoomY, 0.0f, -1.0f, 10.0f);
	}
	else
	{
		float aspectRatio = 1280.0f / 720.0f;
		projection = glm::perspective(angle, -aspectRatio, 0.001f, 10000.0f);
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