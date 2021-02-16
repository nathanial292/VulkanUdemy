#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

const int MAX_FRAME_DRAWS = 2;
const int MAX_OBJECTS = 20;

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Indicies (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndicies {
	int graphicsFamily = -1; // Location of Graphics Queue Family
	int presentationFamily = -1; // Location of Presentation Queue Family
	bool isValid()
	{
		return graphicsFamily >= 0 && presentationFamily >= 0;
	}
};

struct SwapChainDetails {
	VkSurfaceCapabilitiesKHR surfaceCapabilities; // Surface properties (e.g. image size/extent)
	std::vector<VkSurfaceFormatKHR> formats;	  // Surface Image formats (e.g. RGBA and size of each colour)
	std::vector<VkPresentModeKHR> presentationMode; // How images should be presented to screen
};

struct SwapChainImage {
	VkImage image;
	VkImageView imageView;
};

struct Vertex {
	glm::vec3 pos; // vertex position (x,y,z)
	glm::vec3 col; // vertex color (r,g,b)
	glm::vec2 tex; // Texture coords (u, v)
	glm::vec3 normal; // Normals
};

// Validation layers for Vulkan
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = false;
#endif

// Callback function for validation debugging (will be called when validation information record)
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	std::cerr << "validation layer:" << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

static std::vector<char> readFile(const std::string &fileName) {
	// Open stream from given file
	// std::ios::binary tells stream to read as binary
	// std::ios::ate tells stream to start reading from end of file
	std::ifstream file(fileName, std::ios::binary | std::ios::ate);
	
	// Check if file stream successfully opened
	if (!file.is_open())
	{
		throw std::runtime_error("Failed to open a file");
	}

	size_t fileSize = (size_t)file.tellg(); // Get length of the file for creating vector
	std::vector<char> fileBuffer(fileSize); // Create vector with same size as file

	// Reset pointer to the start of file
	file.seekg(0);

	// Read the file data into the buffer (stream "fileSize" in total)  
	file.read(fileBuffer.data(), fileSize);

	// Close the stream
	file.close();

	return fileBuffer;
}

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
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


static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
	VkMemoryPropertyFlags bufferProperties, VkBuffer* buffer, VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferSize; // Size of one vertex * number of vertices
	bufferCreateInfo.usage = bufferUsage; // Multiple types of buffer are possible, use vertex buffer
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Similar to swapchain images can share vertex buffers

	VkResult result = vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a vertex buffer");
	}

	// Get buffer memory requirements
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memoryRequirements);

	// Allocate memory to buffer
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits, // Index of memory type on physical device that has required bit flags
		bufferProperties); // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT: The buffer is visible to the CPU 
						   // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
						   // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: Memory is optimised for GPU usage, only accessible by the gpu and not the host cpu. Can only be interacted by commands

	// Allocate memory to VkDeviceMemory
	result = vkAllocateMemory(device, &memoryAllocInfo, nullptr,  bufferMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory for vertex buffer");
	}

	// Allocate memory to given vertex buffer
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool) {
	// Command buffer to hold transfer commands
	VkCommandBuffer commandBuffer;

	// Command buffer details
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandBufferCount = 1;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;

	// Allocate command buffer from pool
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	// Information to begin the command buffer record
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // We're only using the command buffer once, so set up for one time submit

	// Begin recording transfer commands
	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

static void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer) {
	// End commands
	vkEndCommandBuffer(commandBuffer);

	// Queue submission information
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	// Submit transfer command to transfer queue and wait until finishes
	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	// Free the temporary command buffer back to pool
	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

	// Region of data to copy from and to
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = 0;
	bufferCopyRegion.size = bufferSize;

	// Command to copy src buffer to dst buffer
	vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

	endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool, VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height) {
	// Create buffer
	VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

	VkBufferImageCopy imageRegion = {};
	imageRegion.bufferOffset = 0; // Offset into data
	imageRegion.bufferRowLength = 0; // Row length of data to calculate data spacing
	imageRegion.bufferImageHeight = 0; // Image height to calculate data spacing (tightly packed as 0)
	imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to copy
	imageRegion.imageSubresource.mipLevel = 0; // MIPMAP level to copy
	imageRegion.imageSubresource.layerCount = 1; // Number of layers to copy starting at baseArrayLayer
	imageRegion.imageOffset = { 0,0,0 }; // Offset into image as opposed to raw data in bufferOffset
	imageRegion.imageExtent = { width, height, 1 }; // Size of region to copy (x, y, z) values

	vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

	endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout) {

	// Create Buffer
	VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.oldLayout = oldLayout; // Layout to transition from
	imageMemoryBarrier.newLayout = newLayout; // Layout to transition to
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // Queue family to transition to
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // // From
	imageMemoryBarrier.image = image;
	imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Aspect of image being altered
	imageMemoryBarrier.subresourceRange.baseMipLevel = 0; // First mip level to start alteration on
	imageMemoryBarrier.subresourceRange.layerCount = 1; // Number of layers ot alter starting from baseArrayLayer
	imageMemoryBarrier.subresourceRange.levelCount = 1; // Number of muip levels to alter starting from mipBaseLevel
	imageMemoryBarrier.subresourceRange.baseArrayLayer = 0; // First layer to start alterations on

	VkPipelineStageFlags srcStage;
	VkPipelineStageFlags dstStage;

	// If transitioning from new image to image ready to receive data
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = 0; // Memory access stage transition must after
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; // Memory access stage must before

		srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	// If transitioning from transfer destination to shader readable
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		srcStage, dstStage,  // Pipeline stages (match to src and dst access masks)
		0, // Dependancy flags
		0, nullptr, // Memory barrier count + data
		0, nullptr, // Buffer memory barrier count + data
		1, &imageMemoryBarrier // Image memory barrier count + data
	);

	endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}


static void calcAverageNormals(std::vector<uint32_t>* indices, std::vector<Vertex>* vertices)
{
	// For each face in a mesh, ln0, ln1, ln2 correspond to each vertex of that face.
	// We want to create two new vector

	for (size_t i = 0; i < indices->size(); i += 3)
	{
		unsigned int ln0 = indices->at(i);
		unsigned int ln1 = indices->at(i + 1);
		unsigned int ln2 = indices->at(i + 2);

		//std::cout << ln0 << "\n";
		//std::cout << ln1 << "\n";
		//std::cout << ln2 << "\n";		

		glm::vec3 v1(vertices->at(ln1).pos.x - vertices->at(ln0).pos.x, vertices->at(ln1).pos.y - vertices->at(ln0).pos.y, vertices->at(ln1).pos.z - vertices->at(ln0).pos.z);
		glm::vec3 v2(vertices->at(ln2).pos.x - vertices->at(ln0).pos.x, vertices->at(ln2).pos.y - vertices->at(ln0).pos.y, vertices->at(ln2).pos.z - vertices->at(ln0).pos.z);

		glm::vec3 normal = glm::cross(v1, v2);
		normal = glm::normalize(normal);

		vertices->at(ln0).normal.x += normal.x; vertices->at(ln0).normal.y += normal.y; vertices->at(ln0).normal.z += normal.z;
		vertices->at(ln1).normal.x += normal.x; vertices->at(ln1).normal.y += normal.y; vertices->at(ln1).normal.z += normal.z;
		vertices->at(ln2).normal.x += normal.x; vertices->at(ln2).normal.y += normal.y; vertices->at(ln2).normal.z += normal.z;
	}

	for (size_t i = 0; i < vertices->size(); i++)
	{
		glm::vec3 vec(vertices->at(i).normal.x, vertices->at(i).normal.y, vertices->at(i).normal.z);
		vec = glm::normalize(vec);
		vertices->at(i).normal.x = vec.x;
		vertices->at(i).normal.y = vec.y;
		vertices->at(i).normal.z = vec.z;
	}
}
