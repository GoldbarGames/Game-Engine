#ifndef CAMERA_H
#define CAMERA_H
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <SDL2/SDL.h>

#include "leak_check.h"
class Entity;
class Game;

class KINJO_API Camera
{
private:

	glm::vec3 front = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 0, 0);
	glm::vec3 right = glm::vec3(0, 0, 0);
	glm::vec3 worldUp = glm::vec3(0, 0, 0);

	float movementSpeed = 0;
	float turnSpeed = 0;


public:

	bool isGUI = false;

	float maxZoom = 2.0f;
	float minZoom = 0.6f;

	bool draggable = false;
	bool isDragged = false;

	bool dragOnLeft = false;
	bool dragOnRight = false;
	bool dragOnMiddle = false;

	float dragSpeed = 1.0f;
	float zoomSpeed = 0.1f;

	glm::vec3 dragStartPos = glm::vec3(0, 0, 0);
	glm::vec3 lastMousePos = glm::vec3(0, 0, 0);

	// The up reference Update() derives the camera basis from. Default is
	// (0,1,0); Z-vertical worlds (2.5D games orbiting about world z) need
	// their vertical here or the view rolls when yawed off-axis.
	void SetWorldUp(const glm::vec3& u) { worldUp = u; }

	Camera();
	Camera(glm::vec3 startPos, glm::vec3 startUp, float startYaw, float startPitch,
		float startMovementSpeed, float startTurnSpeed, float startZoom,
		float width, float height, bool useOrtho);
	~Camera();

	void UpdateDrag(const Game& game);

	void KeyControl(const uint8_t* input, const float& dt,
		const float& screenWidth, const float& screenHeight);

	void MouseControl(float xChange, float yChange);

	void Zoom(float amount, float screenWidth, float screenHeight);
	void ResetProjection();
	void ResetCamera();

	// Convenience method to set up a working 3D perspective camera
	void SetupPerspective(float fovDegrees = 60.0f, float nearClip = 0.1f, float farClip = 5000.0f);

	// Projects a world-space point to screen coordinates with a top-left
	// origin (like mouse coordinates). Returns (screenX, screenY, clip.w):
	// the point is in front of the camera only when .z > 0 (when .z <= 0
	// the x/y components are meaningless). Pass the coordinate space size
	// you want to map to (window pixels, or GUI units = 2x window).
	// NOTE: +ndc.y is screen-up even though the 3D projection is Y-flipped;
	// this method encapsulates that so callers never deal with it.
	glm::vec3 WorldToScreenPoint(const glm::vec3& worldPos,
		float screenWidth, float screenHeight) const;

	// Builds a 3D picking ray from a screen position with a top-left origin
	// (like mouse coordinates), in window pixels. Outputs the ray origin on
	// the near plane and its normalized direction.
	void ScreenPointToRay(float screenX, float screenY,
		float screenWidth, float screenHeight,
		glm::vec3& rayOrigin, glm::vec3& rayDirection) const;

	static float MULTIPLIER;

	glm::mat4 projection = glm::mat4();
	glm::mat4 guiProjection = glm::mat4();
	glm::mat4 CalculateViewMatrix() const;

	const SDL_Rect GetBounds() const;

	SDL_Rect dragBounds;

	glm::vec3 position;
	float orthoZoom = 4.0f;
	float angle = -45.0f;
	float yaw = 0;
	float pitch = 0;
	float roll = 0;

	// 3D perspective camera settings
	float fov = 60.0f;        // Field of view in degrees
	float nearPlane = 0.1f;   // Near clipping plane
	float farPlane = 5000.0f; // Far clipping plane
	bool flipY = true;        // Flip Y axis for OpenGL coordinate system
	float startScreenWidth = 1280;
	float startScreenHeight = 720;
	const Entity* target = nullptr;
	int startingTargetID = -1;
	int afterStartingTargetID = -1;
	bool shouldUpdate = true;
	bool useOrthoCamera = true;
	bool switchingTarget = false;
	glm::vec3 startPosition;
	glm::vec3 nextPosition;
	uint32_t startTime = 0;
	uint32_t endTime = 0;
	bool isLerping = false;

	glm::vec3 targetOffset = glm::vec3(0, 0, 0);
	
	void SwitchTarget(const Entity& newTarget);
	void FollowTarget(const Game& game, bool instantFollow=false);
	void Update();

	float startingZoom = 4.0f;

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);
};

#endif

