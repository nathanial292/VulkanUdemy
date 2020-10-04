#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include "Utilities.h"

class Mesh
{
public:
	Mesh();
	Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex> * vertices);
	
	int getVertexCount();
	VkBuffer getVertexBuffer();

	~Mesh();
private:
	int vertexCount;
	VkBuffer vertexBuffer;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkBuffer createVertexBuffer(std::vector<Vertex>* vertices);
};

