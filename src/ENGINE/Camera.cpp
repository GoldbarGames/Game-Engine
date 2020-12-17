#include "Camera.h"
#include "Entity.h"
#include "globals.h"
#include "Game.h"

float Camera::MULTIPLIER = 2.0f;

Camera::Camera(glm::vec3 startPos, glm::vec3 startUp, 
	float startYaw, float startPitch,
	float startMovementSpeed, float startTurnSpeed,
	float startZoom, float width, float height, bool useOrtho)
{
	useOrthoCamera = useOrtho;
	
	position = startPos;
	if (useOrthoCamera)
		position.z = 0;

	startPosition = position;

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
	position = startPosition;
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

void Camera::FollowTarget(const Game& game, bool instantFollow)
{
	if (target != nullptr)
	{
		glm::vec3 targetCenter = glm::vec3(target->GetPosition().x - (startScreenWidth * 0.5f),
			target->GetPosition().y - (startScreenHeight * 0.5f), position.z);

		if (instantFollow)
		{
			position = targetCenter;
		}
		else
		{
			const float scrollFactor = 0.0f;
			const float screenScrollWidth = startScreenWidth * scrollFactor;
			const float screenScrollHeight = startScreenHeight * scrollFactor;

			if (targetCenter.x > position.x + screenScrollWidth ||
				targetCenter.x < position.x - screenScrollWidth)
			{
				isLerping = true;
			}

			if (targetCenter.y > position.y + screenScrollHeight ||
				targetCenter.y < position.y - screenScrollHeight)
			{
				isLerping = true;
			}

			if (isLerping)
			{
				//TODO: Seems like when moving right, the target outpaces the camera for some reason?
				//TODO: This probably needs to be multiplied by game.dt
				glm::vec3 nextPosition = position;
				isLerping = !LerpVector3(nextPosition, targetCenter, 50.0f, 2.0f);

				bool hasCollisions = false;

				// Get bounds assuming the move is valid
				SDL_Rect myBounds;
				myBounds.x = position.x;
				myBounds.y = position.y;
				myBounds.w = startScreenWidth;
				myBounds.h = startScreenHeight;

				//myBounds.x -= (myBounds.w / 2);

				SDL_Rect newBoundsHorizontal = myBounds;
				newBoundsHorizontal.x = nextPosition.x;

				SDL_Rect newBoundsVertical = myBounds;
				newBoundsVertical.y = nextPosition.y;
				newBoundsVertical.y += 1;

				bool horizontalCollision = false;
				bool verticalCollision = false;

				SDL_Rect theirBounds;

				Entity* entity = nullptr;
				for (int i = 0; i < game.cameraBoundsEntities.size(); i++)
				{
					entity = game.cameraBoundsEntities[i];
					theirBounds = *(entity->GetBounds());

					if (!horizontalCollision && HasIntersection(newBoundsHorizontal, theirBounds))
					{
						horizontalCollision = true;
					}

					if (!verticalCollision && HasIntersection(newBoundsVertical, theirBounds))
					{
						verticalCollision = true;
					}
				}

				if (horizontalCollision)
				{
					// only move the camera up to the point
					// where the collision hits					
				}
				else
				{
					position.x = nextPosition.x;
				}

				if (verticalCollision)
				{
					// only move the camera up to the point
					// where the collision hits
				}
				else
				{		
					position.y = nextPosition.y;
				}

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

void Camera::KeyControl(const uint8_t* input, const float& dt, 
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
		float zoomX = (startScreenWidth * startingZoom);
		float zoomY = (startScreenHeight * startingZoom);
		projection = glm::ortho(0.0f, zoomX, zoomY, 0.0f, -1.0f, 10.0f);
	}
	else
	{
		float aspectRatio = startScreenWidth / startScreenHeight;
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