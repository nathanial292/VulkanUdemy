#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"

namespace vulkan {
	struct Model {
		glm::mat4 model;
		glm::mat4 inverseModel;
		bool hasTexture;
		glm::mat3 a;//dynamic descriptors reeee (has to be lined up to minoffset) (it should be but it doesn't work thonk)
		glm::vec4 c;
		glm::vec4 d;
	};

	class Mesh
	{
	public:
		Mesh();
		Mesh(VkPhysicalDevice newPhysicalDevice,
			VkDevice newDevice,
			VkQueue transferQueue,
			VkCommandPool transferCommandPool,
			std::vector<uint32_t>* indices,
			std::vector<Vertex>* vertices,
			int newTexId);

		Mesh(VkPhysicalDevice newPhysicalDevice,
			VkDevice newDevice,
			VkQueue transferQueue,
			VkCommandPool transferCommandPool,
			std::vector<uint32_t>* indices,
			std::vector<Vertex>* vertices);

		int getTexId();

		int getVertexCount();
		VkBuffer getVertexBuffer();

		int getIndexCount();
		VkBuffer getIndexBuffer();

		void destroyBuffers();

		void setModel(glm::mat4 newModel);
		Model getModel();

		void setTexture(bool state)
		{
			model.hasTexture = state;
		}

		~Mesh();
	private:
		Model model;
		int texId;

		int vertexCount;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;

		int indexCount;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		VkPhysicalDevice physicalDevice;
		VkDevice device;

		void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex>* vertices);
		void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t>* indices);
	};
}