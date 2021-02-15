#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"
#include "Window.h"
#include "Camera.h"

class Main {
public:
	Main() {
		gameLoop();
	}

	VulkanRenderer getVulkanRenderer() {
		return vulkanRenderer;
	}

	int gameLoop()
	{
		// Create Camera
		// Start Pos (x,y,z)
		// Start Up (x,y,z)
		// GLfloat startYaw, GLfloat startPitch, GLfloat startMoveSpeed, GLfloat startTurnSpeed
		camera = new Camera(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -10.0f, 5.0f, 0.05f);
#		// Create window
		theWindow = new Window();
		theWindow->Initialise();

		// Create VulkanRenderer Instance
		if (vulkanRenderer.init(theWindow, camera) == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}

		float angle = 0.0f;
		float deltaTime = 0.0f;
		float lastTime = 0.0f;

		// Loop until closed
		while (!theWindow->getShouldClose())
		{
			glfwPollEvents();
			camera->keyControl(theWindow->getKeys(), deltaTime);
			camera->mouseControl(theWindow->getXChange(), theWindow->getYChange());

			float now = glfwGetTime();
			deltaTime = now - lastTime;
			lastTime = now;

			angle += 10.0f * deltaTime;
			if (angle > 360.0f) angle = 0.0f;


			glm::mat4 firstModel(1.0f);
			firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(-0.0f, -1.0f, 0.0f));

			//vulkanRenderer.updateModelMesh(0, firstModel);
			vulkanRenderer.updateModel(0, firstModel);

			vulkanRenderer.draw();
		}

		vulkanRenderer.cleanup();

		// Destory GLFW window and stop GLFW
		theWindow->cleanUp();
		return 0;
	}

private:
	Camera *camera;
	Window *theWindow;

	VulkanRenderer vulkanRenderer;
};

int main() {
	Main main;
}