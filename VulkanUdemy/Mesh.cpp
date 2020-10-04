#include "Mesh.h"

Mesh::Mesh()
{

}

Mesh::Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, std::vector<Vertex>* vertices)
{
	vertexCount = vertices->size();
	physicalDevice = newPhysicalDevice;
	device = newDevice;
	vertexBuffer = createVertexBuffer(vertices);
}

int Mesh::getVertexCount()
{
	return vertexCount;
}

VkBuffer Mesh::getVertexBuffer()
{
	return vertexBuffer;
}

void Mesh::destroyVertexBuffer()
{
	vkDestroyBuffer(device, vertexBuffer, nullptr);
	vkFreeMemory(device, vertexBufferMemory, nullptr);
}

Mesh::~Mesh()
{

}

VkBuffer Mesh::createVertexBuffer(std::vector<Vertex>* vertices)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = sizeof(Vertex) * vertices->size(); // Size of one vertex * number of vertices
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT; // Multiple types of buffer are possible, use vertex buffer
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Similar to swapchain images can share vertex buffers

	VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, &vertexBuffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a vertex buffer");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, vertexBuffer, &memoryRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(memoryRequirements.memoryTypeBits, // Index of memory type on physical device that has required bit flags
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT); // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: The buffer is visible to the CPU 
																					 // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
	// Allocate memory to VkDeviceMemory
	VkResult result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, &vertexBufferMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory for vertex buffer");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
}

uint32_t Mesh::findMemoryTypeIndex(uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
	// Get properties of physical device memory
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if ((allowedTypes & (1 << i)) // Index of memory type must match corresponding bit in allowedTypes
		&& (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) // Desired property bit flags are part of memory type's property flags
		{
			return i; // Memory type is valid, return index 
		}
	}
}
