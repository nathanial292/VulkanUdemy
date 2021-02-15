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
		// Populate meshList and modelList with vertices
		CreateObjects();

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

		/*
				light = DirectionalLight(1.0f, 1.0f, 1.0f,
			0.1f, 0.8f,
			0.0f, 6.0f, 0.0f);
		*/


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

	void CreateObjects() {
		std::vector<Vertex> meshVertices2 = {
			{ { -2, 2, -2.0 },{ 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f} }, // 0
			{ { -2, -0.1, -2.0 },{ 0.0f, 1.0f, 0.0f }, {1.0f, 0.0f} }, // 1
			{ { 2, -0.1, -2.0 },{ 0.0f, 0.0f, 1.0f }, {0.0f, 0.0f} }, // 2
			{ { 2, 2, -2.0 },{ 1.0f, 1.0f, 0.0f }, {0.0f, 1.0f} }, // 3
		};
		// Index data
		std::vector<uint32_t> meshIndicies2 = {
			0, 1, 2,
			2, 3, 0
		};

		std::vector<Vertex> floorVertices = {
			{ { -20.0, 0.0, -20.0}, { 0.0f, 0.0f, 0.0f}, { 1.0f, 1.0f } }, //BL
			{ { 20.0, 0.0, -20.0}, { 10.0f, 0.0f, 0.0f}, { 1.0f, 0.0f } },//BR
			{ { -20., 0.0, 20.0 }, { 0.0f, 10.0f, 0.0f }, { 0.0f, 0.0f } },//FL
			{ { 20.0, 0.0, 20.0 }, { 10.0f, 10.0f, 0.0f }, { 0.0f, 1.0f } }//FR
		};

		std::vector<uint32_t> floorIndices = {
			0, 2, 1,
			1, 2, 3
		};

		Mesh firstMesh = Mesh(vulkanRenderer.getVulkanDevice().physicalDevice, vulkanRenderer.getVulkanDevice().logicalDevice, vulkanRenderer.getGraphicsQueue(), vulkanRenderer.getGraphicsCommandPool(), &floorIndices, &floorVertices, vulkanRenderer.createTexture("marble.jpg"));
		Mesh secondMesh = Mesh(vulkanRenderer.getVulkanDevice().physicalDevice, vulkanRenderer.getVulkanDevice().logicalDevice, vulkanRenderer.getGraphicsQueue(), vulkanRenderer.getGraphicsCommandPool(), &meshIndicies2, &meshVertices2, vulkanRenderer.createTexture("marble.jpg"));

		meshList.push_back(firstMesh);
		meshList.push_back(secondMesh);

		MeshModel meshModel = vulkanRenderer.createMeshModel("models/chair_01.obj", vulkanRenderer.createTexture("wood.png"));
		modelList.push_back(meshModel);

		for (size_t i = 0; i <= MAX_FRAME_DRAWS; ++i) {
			vulkanRenderer.updateUniformBuffers(i);
		}

		vulkanRenderer.setModelList(&modelList);
		vulkanRenderer.setMeshList(&meshList);
	}


private:
	Camera *camera;
	Window *theWindow;
	//DirectionalLight light;
	std::vector<Mesh> meshList;
	std::vector<MeshModel> modelList;

	VulkanRenderer vulkanRenderer;
};

int main() {
	Main main;
}