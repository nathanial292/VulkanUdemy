#pragma once

#include <vector>
#include <iostream>

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

// Validation layers for Vulkan
const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
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