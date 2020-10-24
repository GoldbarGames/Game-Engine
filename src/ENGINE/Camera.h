#ifndef CAMERA_H
#define CAMERA_H
#pragma once

#include <SDL_stdinc.h>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <GLFW/glfw3.h>

class Entity;
class Game;

class Camera
{
public:
	Camera();
	Camera(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch,
		float startMovementSpeed, float startTurnSpeed, float startZoom,
		float width, float height, bool useOrtho);
	~Camera();

	void KeyControl(const Uint8* input, const float& dt,
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


private:
	
	glm::vec3 front = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 0, 0);
	glm::vec3 right = glm::vec3(0, 0, 0);
	glm::vec3 worldUp = glm::vec3(0, 0, 0);

	float startingZoom = 4.0f;
	float movementSpeed = 0;
	float turnSpeed = 0;
};

#endif
