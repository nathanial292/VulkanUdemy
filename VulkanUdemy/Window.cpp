#include "Window.h"
#include <iostream>
#include <glm/glm.hpp>

Window::Window()
{
	width = 800;
	height = 600;
}

Window::Window(GLint windowWidth, GLint windowHeight)
{
	width = windowWidth;
	height = windowHeight;
}

int Window::Initialise()
{
	// Initialse GLFW
	if (!glfwInit())
	{
		printf("Error Initialising GLFW");
		glfwTerminate();
		return 1;
	}

	// Set GLFW to not work with opengl
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

	mainWindow = glfwCreateWindow(width, height, "Test window", nullptr, nullptr);
	if (!mainWindow)
	{
		printf("Error creating GLFW window!");
		glfwTerminate();
		return 1;
	}

	glfwSetWindowUserPointer(mainWindow, this);
	glfwSetFramebufferSizeCallback(mainWindow, framebufferResizeCallback);
	glfwSetInputMode(mainWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(mainWindow, mouse_callback);
	glfwSetKeyCallback(mainWindow, key_callback);

	return 0;
}

GLfloat Window::getXChange()
{
	GLfloat change = xChange;
	xChange = 0.0f;
	return change;
}

GLfloat Window::getYChange()
{
	GLfloat change = yChange;
	yChange = 0.0f;
	return change;
}

Window::~Window()
{
	glfwDestroyWindow(mainWindow);
	glfwTerminate();
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			theWindow->keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			theWindow->keys[key] = false;
		}
	}
}

void Window::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	Window* theWindow = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (theWindow->mouseFirstMoved)
	{
		theWindow->lastX = xpos;
		theWindow->lastY = ypos;
		theWindow->mouseFirstMoved = false;
	}
	theWindow->xChange = xpos - theWindow->lastX;
	theWindow->yChange = theWindow->lastY - ypos;

	theWindow->lastX = xpos;
	theWindow->lastY = ypos;
}

void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	auto app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	app->frameBufferResized = true;
}