#define STB_IMAGE_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>
#include <iostream>

#include "VulkanRenderer.h"

class Main {
public:
	Main() {
		gameLoop();
	}

	VulkanRenderer getVulkanRenderer() {
		return vulkanRenderer;
	}


	void initWindow(std::string wName = "Test Window", const int width = 800, const int height = 600)
	{
		// Initialse GLFW
		glfwInit();

		// Set GLFW to not work with opengl
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, wName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<Main*>(glfwGetWindowUserPointer(window));
		VulkanRenderer renderer = app->getVulkanRenderer();
		renderer.setFrameBufferResize(true);
	}

	int gameLoop()
	{
		// Create window
		initWindow("Test Window", 800, 600);

		// Create VulkanRenderer Instance
		if (vulkanRenderer.init(window) == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}

		float angle = 0.0f;
		float deltaTime = 0.0f;
		float lastTime = 0.0f;

		// Loop until closed
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();

			float now = glfwGetTime();
			deltaTime = now - lastTime;
			lastTime = now;

			angle += 10.0f * deltaTime;
			if (angle > 360.0f) angle = 0.0f;

			glm::mat4 firstModel(1.0f);
			glm::mat4 secondModel(1.0f);

			firstModel = glm::translate(firstModel, glm::vec3(-0.3f, 0.0f, 0.7f));
			firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(-0.0f, 0.0f, -1.0f));

			secondModel = glm::translate(secondModel, glm::vec3(0.7f, 0.0f, -2.0f));
			secondModel = glm::rotate(secondModel, glm::radians(-angle * 10), glm::vec3(0.0f, 0.0f, -1.0f));

			vulkanRenderer.updateModel(0, firstModel);
			vulkanRenderer.updateModel(1, secondModel);

			vulkanRenderer.draw();
		}

		vulkanRenderer.cleanup();

		// Destory GLFW window and stop GLFW
		glfwDestroyWindow(window);
		glfwTerminate();


		return 0;
	}

private:
	GLFWwindow* window;
	VulkanRenderer vulkanRenderer;
};

int main() {
	Main main;
}