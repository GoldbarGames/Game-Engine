#pragma once

#include <SDL_stdinc.h>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <GLFW/glfw3.h>

class Entity;

class Camera
{
public:
	Camera();
	Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch,
		GLfloat startMovementSpeed, GLfloat startTurnSpeed, GLfloat startZoom, float width, float height);
	~Camera();

	void KeyControl(const Uint8* input, GLfloat dt);
	void MouseControl(GLfloat xChange, GLfloat yChange);
	glm::mat4 projection;
	glm::mat4 CalculateViewMatrix();
	void Zoom(float amount, float screenWidth, float screenHeight);

	glm::vec3 position;

	Entity* target = nullptr;

	void FollowTarget();

private:
	
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	float orthoZoom = 4.0f;
	bool useOrthoCamera = true;

	float screenWidth = 1280;
	float screenHeight = 720;
	

	GLfloat yaw;
	GLfloat pitch;
	//GLfloat roll;

	GLfloat movementSpeed;
	GLfloat turnSpeed;

	void Update();

};

