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
	glm::mat4 CalculateViewMatrix();

	glm::vec3 position;
	float orthoZoom = 4.0f;
	float angle = -45.0f;
	float yaw = 0;
	float pitch = 0;
	float roll = 0;
	Entity* target = nullptr;
	bool shouldUpdate = true;
	bool useOrthoCamera = true;
	
	void FollowTarget(const float& screenWidth, const float& screenHeight);
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

