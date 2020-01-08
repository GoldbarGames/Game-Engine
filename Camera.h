#pragma once

#include <SDL_stdinc.h>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <GLFW/glfw3.h>

class Camera
{
public:
	Camera();
	Camera(glm::vec3 startPosition, glm::vec3 startUp, GLfloat startYaw, GLfloat startPitch,
		GLfloat startMovementSpeed, GLfloat startTurnSpeed);
	~Camera();

	void KeyControl(const Uint8* input, GLfloat dt);
	void MouseControl(GLfloat xChange, GLfloat yChange);

	glm::mat4 CalculateViewMatrix();

private:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;

	GLfloat yaw;
	GLfloat pitch;
	//GLfloat roll;

	GLfloat movementSpeed;
	GLfloat turnSpeed;

	void Update();

};

