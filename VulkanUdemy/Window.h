#pragma once

#include <stdio.h>
#include <GLFW\glfw3.h>
#include <glm/glm.hpp>

class Window
{
public:
	Window();

	Window(GLint windowWidth, GLint windowHeight);

	int Initialise();

	GLint getBufferWidth() { return bufferWidth; }
	GLint getBufferHeight() { return bufferHeight; }

	bool getShouldClose() { return glfwWindowShouldClose(mainWindow); }

	void swapBuffers() { glfwSwapBuffers(mainWindow); }

	bool* getKeys() { return keys; }
	GLfloat getXChange();
	GLfloat getYChange();

	void setFrameBufferResize(bool flag) {
		frameBufferResized = flag;
	}
	bool getFrameBufferResized() { return frameBufferResized; }

	GLFWwindow* getWindow() { return mainWindow; }

	void cleanUp() {
		glfwDestroyWindow(mainWindow);
		glfwTerminate();
	}

	~Window();

private:
	GLFWwindow* mainWindow;

	GLint width, height;
	GLint bufferWidth, bufferHeight;

	bool frameBufferResized = false;

	bool keys[1024] = { 0 };

	GLfloat lastX;
	GLfloat lastY;
	GLfloat xChange = 0.0f;
	GLfloat yChange = 0.0f;

	bool mouseFirstMoved = true;
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
};