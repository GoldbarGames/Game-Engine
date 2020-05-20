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
	Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch,
		GLfloat startMovementSpeed, GLfloat startTurnSpeed, GLfloat startZoom, 
		float width, float height, bool useOrtho);
	~Camera();

	void KeyControl(const Uint8* input, const float& dt,
		const float& screenWidth, const float& screenHeight);

	void MouseControl(GLfloat xChange, GLfloat yChange);
	glm::mat4 projection = glm::mat4();
	glm::mat4 guiProjection = glm::mat4();
	glm::mat4 CalculateViewMatrix();
	void Zoom(float amount, float screenWidth, float screenHeight);
	void ResetProjection();
	void ResetCamera();

	glm::vec3 position;

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

	float orthoZoom = 4.0f;
	float startingZoom = 4.0f;
	
	float angle = -45.0f;

	GLfloat yaw = 0;
	GLfloat pitch = 0;
	//GLfloat roll;

	GLfloat movementSpeed = 0;
	GLfloat turnSpeed = 0;


};

