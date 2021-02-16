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
#include "DirectionalLight.h"

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

		light = DirectionalLight(1.0f, 1.0f, 1.0f,
			0.05f, 0.5f,
			8.0f, 10.0f, 8.0f);

		// Create VulkanRenderer Instance
		if (vulkanRenderer.init(theWindow, camera) == EXIT_FAILURE)
		{
			return EXIT_FAILURE;
		}

		// Populate meshList and modelList with vertices
		CreateObjects();

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
			firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
			vulkanRenderer.updateModel(0, firstModel);

			firstModel = glm::mat4(1.0f); // Identity matrix
			firstModel = glm::translate(firstModel, glm::vec3(0.0f, 2.0f, -4.0f));
			firstModel = glm::rotate(firstModel, glm::radians(angle), glm::vec3(-0.0f, -1.0f, 0.0f));
			vulkanRenderer.updateModelMesh(1, firstModel);

			vulkanRenderer.draw();
		}

		vulkanRenderer.cleanup();

		// Destory GLFW window and stop GLFW
		theWindow->cleanUp();
		return 0;
	}

	void CreateObjects() {
		std::vector<Vertex> meshVerticesOld = {
			{ { -2, 2, -2.0 },{ 1.0f, 0.0f, 0.0f }, {1.0f, 1.0f}, { 0.0f, 0.0f, 0.0f} }, // 0
			{ { -2, -0.1, -2.0 },{ 0.0f, 1.0f, 0.0f }, {1.0f, 0.0f}, { 0.0f, 0.0f, 0.0f } }, // 1
			{ { 2, -0.1, -2.0 },{ 0.0f, 0.0f, 1.0f }, {0.0f, 0.0f}, { 0.0f, 0.0f, 0.0f } }, // 2
			{ { 2, 2, -2.0 },{ 1.0f, 1.0f, 0.0f }, {0.0f, 1.0f}, { 0.0f, 0.0f, 0.0f } } // 3

		};

		// Index data
		std::vector<uint32_t> meshIndicesOld = {
			0, 1, 2,
			2, 3, 0
		};

		std::vector<Vertex> meshVertices = {

			{ { -1.0, -1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 0.0, 0.0 }}, // 0
			{ { 1.0, -1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 0.0, 0.0 }},
			{ { -1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, 0.0 }},
			{ { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 1.0 }, { 0.0, 0.0, 0.0 }},


			{ { 1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 0.0, 0.0 }}, // 4
			{ { 1.0, -1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 0.0, 0.0 }},
			{ { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, 0.0 }},
			{ { 1.0, 1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 1.0 }, { 0.0, 0.0, 0.0 }},

			{ { 1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 0.0, 0.0 }}, // 8
			{ { -1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 0.0, 0.0 }},
			{ { 1.0, 1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 1.0 }, { 0.0, 0.0, 0.0 }},
			{ { -1.0, 1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, 0.0 }},

			{ { -1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 0.0, 0.0 }}, // 12
			{ { -1.0, -1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 0.0, 0.0 }},
			{ { -1.0, 1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, 0.0 }},
			{ { -1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 1.0 }, { 0.0, 0.0, 0.0 }},

			{ { -1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, 0.0 }}, // 16
			{ { 1.0, -1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 0.0, 0.0 }},
			{ { -1.0, -1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 1.0 }, { 0.0, 0.0, 0.0 }},
			{ { 1.0, -1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 0.0, 0.0 }},

			{ { -1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 0.0 }, { 0.0, 0.0, 0.0 }}, // 20
			{ { 1.0, 1.0, 1.0 }, { 1.0, 1.0, 1.0 }, { 0.0, 1.0 }, { 0.0, 0.0, 0.0 }},
			{ { -1.0, 1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 0.0 }, { 0.0, 0.0, 0.0 }},
			{ { 1.0, 1.0, -1.0 }, { 1.0, 1.0, 1.0 }, { 1.0, 1.0 }, { 0.0, 0.0, 0.0 }},
		};

		std::vector<uint32_t> meshIndices = {
			1,3,2, 2,0,1,  //Face front
			4,7,6, 6,5,4, //Face right
			10,8,9, 9,11,10, // Back
			14,12,13, 13,15,14, // Left
			16,17,19, 19,18,16, // Bottom
			23,22,20, 20,21,23, // Top
		};


		std::vector<Vertex> floorVertices = {
			{ { -40, 0, -40}, { 0.0f, 0.0f, 0.0f}, { 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f} }, //BL
			{ { 40, 0, -40}, { 10.0f, 0.0f, 0.0f}, { 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f} },//BR
			{ { -40, 0, 40 }, { 0.0f, 10.0f, 0.0f }, { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f} },//FL
			{ { 40, 0, 40 }, { 10.0f, 10.0f, 0.0f }, { 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f} }//FR
		};

		std::vector<uint32_t> floorIndices = {
			0, 2, 1,
			1, 2, 3
		};

		VkPhysicalDevice physicalDevice = vulkanRenderer.getVulkanDevice().physicalDevice;
		VkDevice logicalDevice = vulkanRenderer.getVulkanDevice().logicalDevice;

		calcAverageNormals(&floorIndices, &floorVertices);
		calcAverageNormals(&meshIndices, &meshVertices);

		std::cout << "FRONT FACE" << "\n";

		std::cout << meshVertices[0].normal.x << " " << meshVertices[0].normal.y << " " << meshVertices[0].normal.z << "\n";
		std::cout << meshVertices[1].normal.x << " " << meshVertices[1].normal.y << " " << meshVertices[1].normal.z << "\n";
		std::cout << meshVertices[2].normal.x << " " << meshVertices[2].normal.y << " " << meshVertices[2].normal.z << "\n";
		std::cout << meshVertices[3].normal.x << " " << meshVertices[3].normal.y << " " << meshVertices[3].normal.z << "\n";

		std::cout << "RIGHT FACE" << "\n";

		std::cout << meshVertices[4].normal.x << " " << meshVertices[4].normal.y << " " << meshVertices[4].normal.z << "\n";
		std::cout << meshVertices[5].normal.x << " " << meshVertices[5].normal.y << " " << meshVertices[5].normal.z << "\n";
		std::cout << meshVertices[6].normal.x << " " << meshVertices[6].normal.y << " " << meshVertices[6].normal.z << "\n";
		std::cout << meshVertices[7].normal.x << " " << meshVertices[7].normal.y << " " << meshVertices[7].normal.z << "\n";

		Mesh firstMesh = Mesh(physicalDevice, logicalDevice, vulkanRenderer.getGraphicsQueue(), vulkanRenderer.getGraphicsCommandPool(), &floorIndices, &floorVertices, vulkanRenderer.createTexture("marble.jpg"));
		Mesh secondMesh = Mesh(physicalDevice, logicalDevice, vulkanRenderer.getGraphicsQueue(), vulkanRenderer.getGraphicsCommandPool(), &meshIndices, &meshVertices, vulkanRenderer.createTexture("wood.png"));

		meshList.push_back(firstMesh);
		meshList.push_back(secondMesh);

		MeshModel meshModel = vulkanRenderer.createMeshModel("models/chair_01.obj", vulkanRenderer.createTexture("wood.png"));
		modelList.push_back(meshModel);

		for (size_t i = 0; i <= MAX_FRAME_DRAWS; ++i) {
			vulkanRenderer.updateUniformBuffers(i);
		}

		vulkanRenderer.setModelList(&modelList);
		vulkanRenderer.setMeshList(&meshList);

		vulkanRenderer.setDirectionalLight(light);
	}


private:
	Camera *camera;
	Window *theWindow;
	std::vector<Mesh> meshList;
	std::vector<MeshModel> modelList;
	DirectionalLight light;

	VulkanRenderer vulkanRenderer;
};

int main() {
	Main main;
}