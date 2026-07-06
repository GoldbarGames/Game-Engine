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

	dragBounds.x = -9999;
	dragBounds.y = -9999;
	dragBounds.w = dragBounds.x * -2;
	dragBounds.h = dragBounds.y * -2;

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

void Camera::SetupPerspective(float fovDegrees, float nearClip, float farClip)
{
	// Store settings
	useOrthoCamera = false;
	fov = fovDegrees;
	nearPlane = nearClip;
	farPlane = farClip;
	flipY = true;

	// Calculate and set projection matrix
	float aspectRatio = startScreenWidth / startScreenHeight;
	projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
	projection[1][1] *= -1.0f;  // Y-flip for OpenGL

	// Initialize camera orientation vectors
	shouldUpdate = true;
	Update();
}

glm::vec3 Camera::WorldToScreenPoint(const glm::vec3& worldPos,
	float screenWidth, float screenHeight) const
{
	glm::vec4 clip = projection * CalculateViewMatrix() * glm::vec4(worldPos, 1.0f);

	// Behind the camera; x/y are meaningless here
	if (clip.w <= 0.0f)
		return glm::vec3(0.0f, 0.0f, clip.w);

	glm::vec3 ndc = glm::vec3(clip) / clip.w;

	// +ndc.y is screen-up, screen coordinates grow downward from the top-left
	float sx = (ndc.x * 0.5f + 0.5f) * screenWidth;
	float sy = (0.5f - ndc.y * 0.5f) * screenHeight;
	return glm::vec3(sx, sy, clip.w);
}

void Camera::ScreenPointToRay(float screenX, float screenY,
	float screenWidth, float screenHeight,
	glm::vec3& rayOrigin, glm::vec3& rayDirection) const
{
	// Screen (top-left origin) -> NDC; +ndc.y is screen-up, so the vertical
	// axis flips here
	float nx = (screenX / screenWidth) * 2.0f - 1.0f;
	float ny = 1.0f - (screenY / screenHeight) * 2.0f;

	glm::mat4 inv = glm::inverse(projection * CalculateViewMatrix());
	glm::vec4 p0 = inv * glm::vec4(nx, ny, -1.0f, 1.0f);  // near plane
	glm::vec4 p1 = inv * glm::vec4(nx, ny, 1.0f, 1.0f);   // far plane
	p0 /= p0.w;
	p1 /= p1.w;

	rayOrigin = glm::vec3(p0);
	rayDirection = glm::normalize(glm::vec3(p1) - glm::vec3(p0));
}

void Camera::SwitchTarget(const Entity& newTarget)
{
	switchingTarget = true;
	target = &newTarget;
	nextPosition = glm::vec3(target->GetPosition().x - (startScreenWidth * 0.5f),
		target->GetPosition().y - (startScreenHeight * 0.5f), position.z);
}

const SDL_Rect Camera::GetBounds() const
{
	SDL_Rect cameraBounds;

	// TODO: Don't hardcode these numbers
	if (Globals::TILE_SIZE >= 32)
	{
		cameraBounds.x = position.x - (6 * Globals::TILE_SIZE);
		cameraBounds.y = position.y - (6 * Globals::TILE_SIZE);
		cameraBounds.w = (32 * Globals::TILE_SIZE) * Camera::MULTIPLIER;
		cameraBounds.h = (20 * Globals::TILE_SIZE) * Camera::MULTIPLIER;
	}
	else
	{
		cameraBounds.x = position.x - (12 * Globals::TILE_SIZE);
		cameraBounds.y = position.y - (12 * Globals::TILE_SIZE);
		cameraBounds.w = (64 * Globals::TILE_SIZE) * Camera::MULTIPLIER;
		cameraBounds.h = (40 * Globals::TILE_SIZE) * Camera::MULTIPLIER;
	}

	return cameraBounds;
}

void Camera::FollowTarget(const Game& game, bool instantFollow)
{
	if (!isDragged && target != nullptr)
	{
		glm::vec3 targetCenter = glm::vec3(target->GetPosition().x - (startScreenWidth * 0.5f),
			target->GetPosition().y - (startScreenHeight * 0.5f), position.z);
		
		
		// TODO: Why does this get stuck? It used to work just fine.
		instantFollow = true;

		if (instantFollow)
		{
			position = targetCenter + targetOffset;
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
				for (size_t i = 0; i < game.cameraBoundsEntities.size(); i++)
				{
					entity = game.cameraBoundsEntities[i];
					
					if (entity != nullptr)
					{
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

void Camera::UpdateDrag(const Game& game)
{
	const glm::vec3 curMousePos = game.inputManager.GetMouseWorldPos(game);

	// Get the distance the mouse has moved between frames and add it to the initial position
	const glm::vec3 diffMousePos = (curMousePos - lastMousePos) * dragSpeed;

	SDL_Rect point;
	point.x = dragStartPos.x - diffMousePos.x;
	point.y = dragStartPos.y - diffMousePos.y;
	point.w = 1;
	point.h = 1;

	if (HasIntersection(point, dragBounds))
	{
		position = dragStartPos - diffMousePos;
	}	
}

void Camera::MouseControl(float xChange, float yChange)
{
	if (useOrthoCamera)
	{
		return;
	}

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

	if (!isGUI)
	{
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

			Update();
		}

		if (input[SDL_SCANCODE_Y])
		{
			if (!shouldUpdate)
				return;

			//yaw += xChange;
			pitch -= 1.0f;

			if (pitch < -89.0f)
				pitch = -89.0f;

			Update();
		}


		if (input[SDL_SCANCODE_G])
		{
			if (!shouldUpdate)
				return;

			yaw += 1.0f;

			if (yaw > 89.0f)
				yaw = 89.0f;

			Update();
		}

		if (input[SDL_SCANCODE_H])
		{
			if (!shouldUpdate)
				return;

			yaw -= 1.0f;

			if (yaw < -89.0f)
				yaw = -89.0f;

			Update();
		}

		if (input[SDL_SCANCODE_R])
		{
			if (!shouldUpdate)
				return;

			roll += 1.0f;

			if (roll > 89.0f)
				roll = 89.0f;

			Update();
		}

		if (input[SDL_SCANCODE_T])
		{
			if (!shouldUpdate)
				return;

			roll -= 1.0f;

			if (roll < -89.0f)
				roll = -89.0f;

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
	orthoZoom = startingZoom;

	if (useOrthoCamera)
	{
		float zoomX = (startScreenWidth * startingZoom);
		float zoomY = (startScreenHeight * startingZoom);
		projection = glm::ortho(0.0f, zoomX, zoomY, 0.0f, -1.0f, 10.0f);
	}
	else
	{
		// Use fov (degrees), proper positive aspect ratio, and near/far planes
		float aspectRatio = startScreenWidth / startScreenHeight;
		projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		if (flipY)
		{
			projection[1][1] *= -1.0f;  // Y-flip for OpenGL
		}
	}
}

void Camera::Zoom(float amount, float screenWidth, float screenHeight)
{
	orthoZoom = orthoZoom + amount;


	// TODO: Make this so editor (guiCamera) can zoom in/out indefinitely

/*

	if (orthoZoom > maxZoom)
	{
		orthoZoom = maxZoom;
	}
	else if (orthoZoom < minZoom)
	{
		orthoZoom = minZoom;
	}

*/

	// TODO: Is this comment still accurate?
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
		// Use fov (degrees), proper positive aspect ratio, and near/far planes
		float aspectRatio = screenWidth / screenHeight;
		projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		if (flipY)
		{
			projection[1][1] *= -1.0f;  // Y-flip for OpenGL
		}
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


void Camera::Save(std::unordered_map<std::string, std::string>& map)
{
	map["id"] = "0";
	map["type"] = "camera";
	map["positionX"] = std::to_string((float)position.x);
	map["positionY"] = std::to_string((float)position.y);
	map["positionZ"] = std::to_string((float)position.z);
	map["pitch"] = std::to_string((float)pitch);
	map["yaw"] = std::to_string((float)yaw);
	map["roll"] = std::to_string((int)roll);
	map["angle"] = std::to_string((float)angle);
	map["zoom"] = std::to_string((float)startingZoom);

	// We don't want to accidentally save changes to the camera
	// in the middle of moving the camera around to look at the level.
	// So in order to change things like startingZoom,
	// we click on a button that brings up a list of properties
	// that we can manually type in to change.

	map["targetID"] = std::to_string(startingTargetID);
	map["nextID"] = std::to_string(afterStartingTargetID);
}

void Camera::Load(std::unordered_map<std::string, std::string>& map, Game& game)
{
	startingZoom = std::stof(map["zoom"]);
	startingTargetID = std::stoi(map["targetID"]);

	// if ID == p, find the player
	if (map["nextID"] == "p")
	{
		for (size_t i = 0; i < game.entities.size(); i++)
		{
			if (game.entities[i]->etype == "player")
			{
				afterStartingTargetID = game.entities[i]->id;
				break;
			}
		}

		if (map["nextID"] == "p")
		{
			afterStartingTargetID = -1;
		}
	}
	else
	{
		afterStartingTargetID = std::stoi(map["nextID"]);
	}

	try
	{
		/*
		position.x = std::stof(map["positionX"]);
		position.y = std::stof(map["positionY"]);
		position.z = std::stof(map["positionZ"]);

		yaw = std::stof(map["yaw"]);
		pitch = std::stof(map["pitch"]);
		roll = std::stof(map["roll"]);
		angle = std::stof(map["angle"]);
		*/
	}
	catch (std::exception e)
	{
		game.logger.Log(e.what());
	}

	ResetProjection();

	Update();

	std::cout << GetBounds().x << "," << GetBounds().y << "," << GetBounds().w << "," << GetBounds().h << std::endl;

}