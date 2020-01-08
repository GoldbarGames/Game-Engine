#pragma once

#include <GL/glew.h>
//#include <GLFW/glfw3.h>

/*

class Window
{
public:
	Window();
	Window(GLint windowWidth, GLint windowHeight);
	~Window();

	int Init();

	GLfloat getBufferWidth() { return bufferWidth; }
	GLfloat getBufferHeight() { return bufferHeight; }
	GLfloat calcAspectRatio() { return (GLfloat)bufferWidth / (GLfloat)bufferHeight; }

	bool getShouldClose() { return glfwWindowShouldClose(mainWindow); }

	bool* getKeys() { return keys; }

	bool GetKeyDown(int k) { return k >= 0 && k < 1024 && keys[k] && !prevKeys[k]; }

	GLfloat getXChange();
	GLfloat getYChange();

	void swapBuffers() { glfwSwapBuffers(mainWindow); }

private:
	GLFWwindow* mainWindow;

	GLint width, height;
	GLint bufferWidth, bufferHeight;

	bool keys[1024];
	bool prevKeys[1024];

	GLfloat lastX, lastY;
	GLfloat xChange, yChange;
	bool mouseFirstMoved = true;

	void createCallbacks();
	static void handleKeys(GLFWwindow* window, int key, int code, int action, int mode);
	static void handleMouse(GLFWwindow* window, double xPos, double yPos);
};

*/