#ifndef CAMERA_H
#define CAMERA_H
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

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
	Camera();
	Camera(glm::vec3 startPos, glm::vec3 startUp, float startYaw, float startPitch,
		float startMovementSpeed, float startTurnSpeed, float startZoom,
		float width, float height, bool useOrtho);
	~Camera();

	void KeyControl(const uint8_t* input, const float& dt,
		const float& screenWidth, const float& screenHeight);

	void MouseControl(float xChange, float yChange);

	void Zoom(float amount, float screenWidth, float screenHeight);
	void ResetProjection();
	void ResetCamera();

	static float MULTIPLIER;

	glm::mat4 projection = glm::mat4();
	glm::mat4 guiProjection = glm::mat4();
	glm::mat4 CalculateViewMatrix() const;

	glm::vec3 position;
	float orthoZoom = 4.0f;
	float angle = -45.0f;
	float yaw = 0;
	float pitch = 0;
	float roll = 0;
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
	
	void SwitchTarget(const Entity& newTarget);
	void FollowTarget(const Game& game, bool instantFollow=false);
	void Update();

	float startingZoom = 4.0f;

	void Save(std::unordered_map<std::string, std::string>& map);
	void Load(std::unordered_map<std::string, std::string>& map, Game& game);
};

#endif

