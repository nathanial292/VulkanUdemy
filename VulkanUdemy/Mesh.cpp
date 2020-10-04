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
}
