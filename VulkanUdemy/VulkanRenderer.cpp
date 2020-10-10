#include "VulkanRenderer.h"


VulkanRenderer::VulkanRenderer()
{
}

VulkanRenderer::~VulkanRenderer()
{
}

int VulkanRenderer::init(GLFWwindow* newWindow)
{
	window = newWindow;
	try {
		createInstance();
		setupDebugMessenger();
		createSurface();
		getPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createDepthBufferImage();
		createRenderPass();
		createDescriptorSetLayout();
		createPushConstantRange();
		createGraphicsPipeline();
		createFrameBuffers();
		createCommandPool();

		uboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f);
		uboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		uboViewProjection.projection[1][1] *= -1; // Invert the y axis for vulkan (GLM was made for opengl which uses +y as up)

		// Create a mesh
		std::vector<Vertex> meshVertices = {
			{ { -0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f } }, // 0
			{ { -0.4, -0.4, 0.0 },{ 0.0f, 1.0f, 0.0f } }, // 1
			{ { 0.4, -0.4, 0.0 },{ 0.0f, 0.0f, 1.0f } }, // 2
			{ { 0.4, 0.4, 0.0 },{ 1.0f, 1.0f, 0.0f } }, // 3
		};

		std::vector<Vertex> meshVertices2 = {
			{ { -0.25, 0.6, 0.0 },{ 1.0f, 0.0f, 0.0f } }, // 0
			{ { -0.25, -0.6, 0.0 },{ 0.0f, 1.0f, 0.0f } }, // 1
			{ { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f } }, // 2
			{ { 0.25, 0.6, 0.0 },{ 1.0f, 1.0f, 0.0f } }, // 3
		};

		// Index data
		std::vector<uint32_t> meshIndicies = {
			0, 1, 2,
			2, 3, 0
		};

		Mesh firstMesh = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, &meshIndicies, &meshVertices);
		Mesh secondMesh = Mesh(mainDevice.physicalDevice, mainDevice.logicalDevice, graphicsQueue, graphicsCommandPool, &meshIndicies, &meshVertices2);

		meshList.push_back(firstMesh);
		meshList.push_back(secondMesh);

		allocateDynamicBufferTransferSpace();
		createUniformBuffers();
		createCommandBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createSynchronisation();

		for (size_t i = 0; i <= MAX_FRAME_DRAWS; ++i) {
			updateUniformBuffers(i);
		}
	}
	catch (const std::runtime_error& e) {
		printf("ERROR: %s\n", e.what());
		return EXIT_FAILURE;
	}

	return 0;
}

void VulkanRenderer::recreateSwapChain()
{
	cleanupSwapChain();
	createSwapChain();
	createRenderPass();
	createGraphicsPipeline();
	createFrameBuffers();
	createCommandBuffers();
}

void VulkanRenderer::updateModel(int modelId, glm::mat4 newModel)
{
	if (modelId >= meshList.size()) return;
	meshList[modelId].setModel(newModel);
}

void VulkanRenderer::draw()
{	
	// Stop running code until the fence is opened, only opened when the frame is finished drawing
	vkWaitForFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
	vkResetFences(mainDevice.logicalDevice, 1, &drawFences[currentFrame]); // Unsignal fence (close it so other frames can't eneter)

	// GET NEXT IMAGE
	uint32_t imageIndex;
	vkAcquireNextImageKHR(mainDevice.logicalDevice, swapchain, std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame], VK_NULL_HANDLE, &imageIndex);

	recordCommands(imageIndex); // Rerecord commands every draw
	//updateUniformBuffers(imageIndex);

	// SUBMIT COMMAND BUFFER FOR EXECUTION
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1; // Number of semaphores to wait on
	submitInfo.pWaitSemaphores = &imageAvailable[currentFrame]; // List of semaphores to wait on
	VkPipelineStageFlags waitStages[] = {
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
	};
	submitInfo.pWaitDstStageMask = waitStages; // stages to check semaphores at
	submitInfo.commandBufferCount = 1; // Number of command buffers to submit
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex]; // Command buffer to submit
	submitInfo.signalSemaphoreCount = 1; // Number of semaphore to signal
	submitInfo.pSignalSemaphores = &renderFinished[currentFrame]; // Semaphores to signal when command buffer finishes

	// Submit command buffer to queue
	VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, drawFences[currentFrame]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to submit command buffer to queue");
	}

	// PRESENT RENDERED IMAGE TO SCREEN
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1; // Number of semaphores to wait on
	presentInfo.pWaitSemaphores = &renderFinished[currentFrame]; // Semaphores to wait on
	presentInfo.swapchainCount = 1; // Number of swapchains to present to
	presentInfo.pSwapchains = &swapchain; // Swapchains to present images to
	presentInfo.pImageIndices = &imageIndex; // Index of images in swapchains to present

	result = vkQueuePresentKHR(presentationQueue, &presentInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to present image!");
	}

	// Get next frame (use modulo MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
	currentFrame = (currentFrame + 1) % MAX_FRAME_DRAWS;
}

void VulkanRenderer::cleanup()
{
	// Wait until no actions being run on device before cleanup
	vkDeviceWaitIdle(mainDevice.logicalDevice);

	// Clean up all components of the swapchain, render pass, graphics pipeline, command buffers, image views
	cleanupSwapChain();

	// C style memory free for dynamic descriptor sets
	_aligned_free(modelTransferSpace);

	// Cleanup for depth buffer
	vkDestroyImageView(mainDevice.logicalDevice, depthBufferImageView, nullptr);
	vkDestroyImage(mainDevice.logicalDevice, depthBufferImage, nullptr);
	vkFreeMemory(mainDevice.logicalDevice, depthBufferImageMemory, nullptr);

	vkDestroyDescriptorPool(mainDevice.logicalDevice, descriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(mainDevice.logicalDevice, descriptorSetLayout, nullptr);
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		vkDestroyBuffer(mainDevice.logicalDevice, vpUniformBuffer[i], nullptr);
		vkFreeMemory(mainDevice.logicalDevice, vpUniformBufferMemory[i], nullptr);
		vkDestroyBuffer(mainDevice.logicalDevice, modelDUniformBuffer[i], nullptr);
		vkFreeMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[i], nullptr);
	}
	for (size_t i = 0; i < meshList.size(); i++) {
		meshList[i].destroyBuffers();
	}
	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {
		vkDestroySemaphore(mainDevice.logicalDevice, renderFinished[i], nullptr);
		vkDestroySemaphore(mainDevice.logicalDevice, imageAvailable[i], nullptr);
		vkDestroyFence(mainDevice.logicalDevice, drawFences[i], nullptr);
	}
	vkDestroyCommandPool(mainDevice.logicalDevice, graphicsCommandPool, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	if (enableValidationLayers) {
		destroyDebugMessenger(nullptr);
	}
	vkDestroyDevice(mainDevice.logicalDevice, nullptr);
	vkDestroyInstance(instance, nullptr);
}

void VulkanRenderer::cleanupSwapChain() {
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(mainDevice.logicalDevice, framebuffer, nullptr);
	}
	vkFreeCommandBuffers(mainDevice.logicalDevice, graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
	vkDestroyPipeline(mainDevice.logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(mainDevice.logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(mainDevice.logicalDevice, renderPass, nullptr);
	for (auto image : swapChainImages) {
		vkDestroyImageView(mainDevice.logicalDevice, image.imageView, nullptr);
	}
	vkDestroySwapchainKHR(mainDevice.logicalDevice, swapchain, nullptr);
}

void VulkanRenderer::createInstance()
{
	// Information about the application itsself
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan App";				// Custom name of the app
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);	// Custom version of app
	appInfo.pEngineName = "Insert Engine Here";				// Name of your engine
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);		// Custom version for engine
	appInfo.apiVersion = VK_API_VERSION_1_2;

	// Creation information for VkInstance
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Debug Utils for instance error checking (before debug utils has been setup)
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;

	// Include the validation layer names if they are enabled
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else {
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// Create list to hold instance extensions
	uint32_t glfwExtensionCount = 0;			// GLFW may require multiple extensions
	const char** glfwExtensions;				// Extensions passed as array of cstrings, so need pointer (the array) to pointer to (cstring)
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> instanceExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
	createInfo.ppEnabledExtensionNames = instanceExtensions.data();

	// Check instance extensions supported
	if (!checkInstanceExtensionSupport(&instanceExtensions))
	{
		throw std::runtime_error("VkInstance does not support required extensions");
	}

	// Check Validation Layers are supported
	if (enableValidationLayers && !checkValidiationLayerSupport()) {
		throw std::runtime_error("Validation layers requested, but not available");
	}

	// Create instance
	VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create a Vulkan Instance");
	}
}

void VulkanRenderer::createLogicalDevice()
{
	// Physical device is created now, choose the main physical device
	QueueFamilyIndicies indicies = getQueueFamilies(mainDevice.physicalDevice);

	// Vector for queue creation information, and set for family indicies
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> queueFamilyIndicies = { indicies.graphicsFamily, indicies.presentationFamily }; // If they both are the same value, only 1 is stored

	// Queues the logical device needs to create and info to do so
	for (int queueFamilyIndex : queueFamilyIndicies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamilyIndex; // Index of the family to create a queue from
		queueCreateInfo.queueCount = 1;						// Number of queues to create
		float priority = 1.0f;
		queueCreateInfo.pQueuePriorities = &priority;		// Vulkan needs to know how to handle multiple queue families, (1 is the highest priority)

		queueCreateInfos.push_back(queueCreateInfo);
	}

	// Information to create logical device, (sometimes called "device")
	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());  // Number of Queue Create Infos
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();							 // List of Queue Create Infos so device can create required queues
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()); // Number of enabled logical device extensions
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();						 // List of enabled logical device extensions

	// Physical device features the logical device will be using
	VkPhysicalDeviceFeatures deviceFeatures = {};
	//deviceFeatures.depthClamp = VK_TRUE; // use if using depthClampEnable to true
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures; // Physical Device features Logical Device will use

	// Create the logical device for the given phyiscal device
	VkResult result = vkCreateDevice(mainDevice.physicalDevice, &deviceCreateInfo, nullptr, &mainDevice.logicalDevice);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Logical Device");
	}

	// Queues are created at the same time as the device, we need a handle to queues
	vkGetDeviceQueue(mainDevice.logicalDevice, indicies.graphicsFamily, 0, &graphicsQueue); // From Logical Device, of given Queue Family, of Queue Index(0). Store queue reference in the graphicsQueue
	vkGetDeviceQueue(mainDevice.logicalDevice, indicies.presentationFamily, 0, &presentationQueue);
}

void VulkanRenderer::createSurface()
{
	// Create the surface (Creating a surface create info struct, specific to GLFW window type) - returns result
	VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &surface);
	if (result != VK_SUCCESS) throw std::runtime_error("Failed to create surface");


}

void VulkanRenderer::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void VulkanRenderer::setupDebugMessenger()
{
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	populateDebugMessengerCreateInfo(createInfo);

	if (createDebugMessenger(&createInfo, nullptr) != VK_SUCCESS) throw std::runtime_error("Failed to setup debug messenger");
}

VkResult VulkanRenderer::createDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, &debugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void VulkanRenderer::destroyDebugMessenger(VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanRenderer::getPhysicalDevice()
{
	// Enumerate the physical devices the vkInstance can access
	// Get the count of devices
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	// If deviceCount = 0 then no devices support vulkan
	if (deviceCount == 0)
	{
		throw std::runtime_error("Can't find GPUs that suport Vulkan Instance");
	}

	// Get list of physical devices
	std::vector<VkPhysicalDevice> deviceList(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data());

	// TEMP: get first device
	for (const auto& device : deviceList)
	{
		if (checkDeviceSuitable(device))
		{
			mainDevice.physicalDevice = device;
			break;
		}
	}
	
	// Get properties of our new device
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(mainDevice.physicalDevice, &deviceProperties);

	minUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;
}

void VulkanRenderer::allocateDynamicBufferTransferSpace()
{
	// Calculate alignment of model data
	modelUniformAlignment = (sizeof(Model) + minUniformBufferOffset - 1) & ~(minUniformBufferOffset-1);

	// Create space in memory to hold dynamic buffer that is aligned to required alignment and holds MAX_OBJECTS
	modelTransferSpace = (Model*)_aligned_malloc(modelUniformAlignment * MAX_OBJECTS, modelUniformAlignment);
}

bool VulkanRenderer::checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions)
{
	// Check how many extensions vulkan supports, need to get the size before we can populate
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	// Create a list of VkExtensionProperties using count
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

	// Check if given extensions are in list of available extensions
	for (const auto& checkExtension : *checkExtensions)
	{
		bool hasExtension = false;
		for (const auto& extension : extensions)
		{
			if (strcmp(checkExtension, extension.extensionName) == 0)
			{
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension)
		{
			return false;
		}
	}
	return true;
}

bool VulkanRenderer::checkDeviceSuitable(VkPhysicalDevice device)
{
	/*
	// Information about the device itself(ID, name, type, vendor, etc)
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	// Information about what the device can do (geo shader, tess shader, wide lines etc)
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
	*/

	// Check what Queues are supported
	QueueFamilyIndicies indicies = getQueueFamilies(device);

	bool extensionsSupported = checkDeviceExtensionSupport(device);

	bool swapChainValid = false;
	// No point checking if swapchain is valid is the extensions are supported
	if (extensionsSupported) {
		SwapChainDetails swapChainDetails = getSwapChainDetails(device);
		swapChainValid = !swapChainDetails.presentationMode.empty() && !swapChainDetails.formats.empty();
	}


	return indicies.isValid() && extensionsSupported && swapChainValid;
}

bool VulkanRenderer::checkValidiationLayerSupport()
{
	// Get total amount of supported layers from instance
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	// Populate an array with supported layers of size supported layers
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	// Check if the validation layers we wish to use are supported by the instance
	for (const auto& validationLayer : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layer : availableLayers)
		{
			if (strcmp(layer.layerName, validationLayer) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound)
		{
			return false;
		}
	}

	return true;
}

bool VulkanRenderer::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
	// Get length of supported extensions
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	// Populate vector with list of supported extensions
	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

	// If no extensions are supported
	if (extensionCount == 0) return false;

	// Check the supported extensions against the extensions we wish to support (deviceExtensions in Utilities.h)
	for (const auto& deviceExtension : deviceExtensions) {
		bool hasExtension = false;
		for (const auto& extension : extensions) {
			if (strcmp(extension.extensionName, deviceExtension) == 0) {
				hasExtension = true;
				break;
			}
		}
		if (!hasExtension) {
			return false;
		}
	}
	return true;
}

QueueFamilyIndicies VulkanRenderer::getQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndicies indicies;

	// Get all Queue Family Property info for the given device
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

	// Go through each queue family, check if it has atleast 1 of the required types of queue
	int i = 0;
	for (const auto& queueFamily : queueFamilyList)
	{
		// First check if queue family has at least 1 in family, queue can be multiple types. Find through bitwise & with VK_QUEUE_*_BIT to check if it has requried type
		if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			indicies.graphicsFamily = i; // If queue family is valid, get the index
		}

		// Check if the queue family supports presentation
		VkBool32 presentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);
		// Check if queue is presentation type (can be both graphics and presentation)
		if (queueFamily.queueCount > 0 && presentationSupport == true)
		{
			indicies.presentationFamily = i;
		}

		// Check if the queue family indicies are in a valid state. Break
		if (indicies.isValid()) break;
		i++;
	}

	return indicies;
}

SwapChainDetails VulkanRenderer::getSwapChainDetails(VkPhysicalDevice device)
{
	SwapChainDetails swapChainDetails;
	
	// Get the surface capabiltiies of given surface for given physical device
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);

	// Formats
	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
	
	if (formatCount != 0) 
	{
		swapChainDetails.formats.resize(formatCount); // Resize vector to the supported formats for given surface
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	// Presentation Modes
	uint32_t presentationCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

	if (presentationCount != 0) 
	{
		swapChainDetails.presentationMode.resize(presentationCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationMode.data());
	}


	return swapChainDetails;
}

// Best format various based on application specification, will use VK_FORMAT_R8G8B8A8_UNIFORM || VK_FORMAT_B8G8R8A8_UNORM (Backup)
// Color space, SRGB, AdobeRGB etc. Will use VK_COLOR_SPACE_SRGB_NONLINEAR_KHR (SRGB)
VkSurfaceFormatKHR VulkanRenderer::chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) // If all formats are supported (VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_R8G8B8A8_UNORM , VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	// If restricted, search for optimal format
	for (const auto& format : formats)
	{
		// if RGB OR BGR are available
		if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || VK_FORMAT_B8G8R8A8_UNORM) && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return format;
		}
	}
	// If can't find optimal format, then just return first format
	return formats[0];
}

VkPresentModeKHR VulkanRenderer::chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes)
{
	for (const auto& presentationMode : presentationModes)
	{
		if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			return presentationMode;
		}
	}

	// If mailbox doesn't exist, use FIFO as Vulkan spec requires it to be present
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanRenderer::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities)
{
	if (surfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) // If current extend is at numeric limits, the extend van vary. Otherwise it is the size of the window.
	{
		return surfaceCapabilities.currentExtent;
	}
	else {
		// If value can vary, need to set manually
		int width, height;
		glfwGetFramebufferSize(window, &width, &height); // Get width and height from window (glfw)

		// Create new extent using window size
		VkExtent2D newExtent = {};
		newExtent.width = static_cast<uint32_t>(width);
		newExtent.height = static_cast<uint32_t>(height);

		// Surface also defines max and min, make sure inside boundaries by clamping value
		newExtent.width = std::max(surfaceCapabilities.minImageExtent.width, std::min(surfaceCapabilities.maxImageExtent.width, newExtent.width)); // Keeps width within the boundaries
		newExtent.height = std::max(surfaceCapabilities.minImageExtent.height, std::min(surfaceCapabilities.maxImageExtent.height, newExtent.height));

		return newExtent;
	}
}

VkFormat VulkanRenderer::chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
	// Loop through all formats and find a compatible one
	for (VkFormat format : formats) {
		// Get properties for given format on this device
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(mainDevice.physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find a matching format!");
}

void VulkanRenderer::createSwapChain()
{
	SwapChainDetails swapChainDetails = getSwapChainDetails(mainDevice.physicalDevice); // Get the swapchain details so we can pick best settings

	// Find optimal surface values for our swapchain
	VkSurfaceFormatKHR surfaceFormat = chooseBestSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = chooseBestPresentationMode(swapChainDetails.presentationMode);
	VkExtent2D extent = chooseSwapExtent(swapChainDetails.surfaceCapabilities);

	// How many images are in the swapchain? Get 1 more than minimum to allow triple buffering
	uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;
	
	// Check that we haven't gone over maximum image count, if 0 then limitless
	if (swapChainDetails.surfaceCapabilities.maxImageCount > 0 && swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
	{
		imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
	}

	// Creation information for swap chain
	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.surface = surface;
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.imageFormat = surfaceFormat.format;
	swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapChainCreateInfo.presentMode = presentMode;
	swapChainCreateInfo.imageExtent = extent;
	swapChainCreateInfo.minImageCount = imageCount; // Number of images in swap chain (min)
	swapChainCreateInfo.imageArrayLayers = 1; // Number of layers for each image in chain
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // What attachment images will be used as
	swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform; // Transform to perform on swap chain images
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // How to handle blending images with external graphics (e.g. other windows)
	swapChainCreateInfo.clipped = VK_TRUE; // Wheher to clip part of images not in view

	// Get Queue Family Indicies
	QueueFamilyIndicies indicies = getQueueFamilies(mainDevice.physicalDevice);

	// If Graphics and Presentation are different, then swapchain must let images be shared between families
	if (indicies.graphicsFamily != indicies.presentationFamily) {

		uint32_t queueFamilyIndicies[] = {
			(uint32_t)indicies.graphicsFamily,
			(uint32_t)indicies.presentationFamily
		};

		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // Image share handling
		swapChainCreateInfo.queueFamilyIndexCount = 2; // Number of queues to share images between
		swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndicies; // Array of queues to share between
	}
	else {
		// Only 1 queue family
		swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainCreateInfo.queueFamilyIndexCount = 0;
		swapChainCreateInfo.pQueueFamilyIndices = nullptr;
	}

	// If old swapchain has been destroyed, and this one replaces it, then quickly hand over responsibilities.
	swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

	// Create swapchain
	VkResult result = vkCreateSwapchainKHR(mainDevice.logicalDevice, &swapChainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create swapchain");
	}

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	// Get the swapchain images
	uint32_t swapChainImageCount = 0;
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, nullptr);
	std::vector<VkImage> images(swapChainImageCount);
	vkGetSwapchainImagesKHR(mainDevice.logicalDevice, swapchain, &swapChainImageCount, images.data()); 

	for (VkImage image : images) {
		// Store the image handle
		SwapChainImage swapChainImage = {};
		swapChainImage.image = image;
		swapChainImage.imageView = createImageView(image, swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);

		// Add image to swapChainImages List in global namespace
		swapChainImages.push_back(swapChainImage);
	}
}

void VulkanRenderer::createRenderPass()
{
	// ATTACHMENTS
	// Color Attachment of render pass
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = swapChainImageFormat; // Format to use for attachment 
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to write for multisampling
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Describes what to do with attachment before rendering
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // What to do with attachment after rendering
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Describes what to do with stencil before rendering
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Describes what to do with stencil after rendering
	
	// Framebuffer data will be stored as an image, but images can be given different layouts
	// to give optimal use for certain operations
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Image data layout before render pass starts
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Image data layout after render pass (to change to)

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = depthBufferFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// REFERENCES
	// Attachment reference uses an attachment index that refers to the attachment list passed to renderPassCreateInfo
	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0; // Use the first attachment in the list passed to renderPassCreateInfo
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Attachment for the depth stencil
	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1; // At index 1 of our attachments
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	// Information about a particular subpass the render pass is using
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	// Need to determine when layout transitions occur using subpass dependencies
	std::array<VkSubpassDependency, 2> subpassDependencies;
	
	// Conversion from  VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// Transiton must happen after
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL; // Subpss Index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // Pipeline stage
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT; // Stage access mask (memory access)
	// But must happen after...
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = 0;

	// Conversion from  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// Transiton must happen after
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
	// But must happen after...
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = 0;

	std::array<VkAttachmentDescription, 2> renderPassAttachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
	renderPassCreateInfo.pAttachments = renderPassAttachments.data();
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();

	VkResult result = vkCreateRenderPass(mainDevice.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create render pass");
	}
}

void VulkanRenderer::createGraphicsPipeline()
{
	// Read in SPIR-V code of shaders
	auto vertexShaderCode = readFile("shaders/vert.spv");
	auto fragShaderCode = readFile("shaders/frag.spv");

	// Build shader Modules to link to graphics pipeline
	VkShaderModule vertShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

	// SHADER STAGE CREATION INFORMATION
	// Vertex stage creation information
	VkPipelineShaderStageCreateInfo vertShaderCreateInfo = {};
	vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderCreateInfo.module = vertShaderModule;
	vertShaderCreateInfo.pName = "main";

	// Fragment stage creation information
	VkPipelineShaderStageCreateInfo fragShaderCreateInfo = {};
	fragShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderCreateInfo.module = fragShaderModule;
	fragShaderCreateInfo.pName = "main";

	// Create array of shader stages (Requires array)
	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderCreateInfo, fragShaderCreateInfo };

	// How the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0; // Can bind multiple streams of data, this defines which one (first binding as only one string)
	bindingDescription.stride = sizeof(Vertex); // Size of a single vertex object, defines how big each vertex is and how to split them
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // How to move between data after each vertex, 
	//VK_VERTEX_INPUT_RATE_INDEX: Move onto next vertex
	//VK_VERTEX_INPUT_RATE_INSTANCE: Move to a vertex for the next instance (If you have multiple instances of the same object) - draws the first vertex of each

	// How the data for an attribute is defined within a vertex
	std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions;

	// Position attribute
	attributeDescriptions[0].binding = 0; // Which binding the data is at (should be the same as above binding value)
	attributeDescriptions[0].location = 0; // Location in shader where data will be read from
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // Format the data will take (also helps define the size of data)
	attributeDescriptions[0].offset = offsetof(Vertex, pos); // Where the attribute is defined in the data for a single vertex

	// Color Attribute
	attributeDescriptions[1].binding = 0; 
	attributeDescriptions[1].location = 1; 
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; 
	attributeDescriptions[1].offset = offsetof(Vertex, col);

	// Vertex Input
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription; // List of vertex binding descriptions (data spacing/stride information)
	vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // List of vertex attribute descriptions (data format and where to bind to/from)

	// Input Assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Primitive type to assemble verticies
	inputAssembly.primitiveRestartEnable = VK_FALSE; // Allow overriding of "strip" topology to start new primitives

	// Viewport and Scissor
	VkViewport viewport = {};
	viewport.x = 0.0f; // x start coordinate
	viewport.y = 0.0f; // y start coordinate
	viewport.width = (float)swapChainExtent.width; // width of viewport
	viewport.height = (float)swapChainExtent.height; // height of viewport
	viewport.minDepth = 0.0f; // Min framebuffer depth
	viewport.maxDepth = 1.0f; // Max framebuffer depth

	// Create a scissor info struct
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 }; // Offset to use region from
	scissor.extent = swapChainExtent; // Extent to describe region to use, starting at offset

	// Viewport and scissor info struct
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	/*
	// Dyanamic states to enable (Change values in runtime instead of hardcoding pipeline)
	std::vector<VkDynamicState> dyanamicStateEnables;
	dyanamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT); // Dynamic Viewport: Can resize in command buffer with vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	dyanamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR); // Dynamic Scissor: Can resize in command buffer with vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	// Dynamic state creation info
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dyanamicStateEnables.size());
	dynamicStateCreateInfo.pDynamicStates = dyanamicStateEnables.data();
	*/

	// Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizerStateCreateInfo = {};
	rasterizerStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateCreateInfo.depthClampEnable = VK_FALSE; // Change if fragments beyond near/far plane are clipped (default) or clamped to plane
	rasterizerStateCreateInfo.rasterizerDiscardEnable = VK_FALSE; // Whether to discard data and skip rasterizer. Never creates fragements
	rasterizerStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; // How to handle filling points between vertices
	rasterizerStateCreateInfo.lineWidth = 1.0f; // The thickness of the lines when drawn (enable wide lines for anything other than 1.0f)
	rasterizerStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // Winding to determine which side is front
	rasterizerStateCreateInfo.depthBiasEnable = VK_FALSE; // Whether to add depth bias to fragments (good for stopping "shadow achne" in shadow mapping)

	// Multisampling (Disabled for now)
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE; // Enable multisample shading or not
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Number of samples to use per fragment
	
	// Blending
	VkPipelineColorBlendAttachmentState colorState = {};
	colorState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // Mask for what colors to be blended
	colorState.blendEnable = VK_TRUE; // Enable blending
	
	// Blending uses equation: (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
	colorState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorState.colorBlendOp = VK_BLEND_OP_ADD;
	// (new color alpha * new color) + ((1 - new color alpha) * old color)

	// How to blend alpha values ( Replace old value with new alpha)
	colorState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorState.alphaBlendOp = VK_BLEND_OP_ADD;
	// (1 * new alpha) + (0 * old alpha) = new alpha

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorState;

	// Pipeline layout (descriptor sets and push constants)
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	// Create Pipeline Layout
	VkResult result = vkCreatePipelineLayout(mainDevice.logicalDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create pipeline layout");
	}
	
	// Depth Stencil Testing
	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE; // Enable checking depth to determine fragment write
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE; // Enable writing to the depth buffer (to replace old values)
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS; // Comparision operation that allows an overwrite (is in front)
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE; // Depth bounds test: Does the depth value exist between 2 bounds (front, back)
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;


	// Create Graphics Pipeline
	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2; // Number of shader stages (vertex, fragment)
	pipelineCreateInfo.pStages = shaderStages; // List of shader stages
	pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo; // All the fixed function pipeline states
	pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
	pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.pRasterizationState = &rasterizerStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout; // Pipeline layout pipeline should use
	pipelineCreateInfo.renderPass = renderPass; // Render pass description the pipeline is compatible with
	pipelineCreateInfo.subpass = 0; // Subpass of render pass to use with pipeline

	// Pipeline derivatives: Can create multiple pipelines that derive from one another for optimisation
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Existing pipeline to derive from
	pipelineCreateInfo.basePipelineIndex = -1; // Or index of pipeline being created to derive from (in case creating multiple at once) 

	result = vkCreateGraphicsPipelines(mainDevice.logicalDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline");
	}

	// Destroy shader modules after pipeline (no longer needed after pipeline created)
	vkDestroyShaderModule(mainDevice.logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(mainDevice.logicalDevice, fragShaderModule, nullptr);
}

void VulkanRenderer::createDepthBufferImage()
{
	// Get supported format for depth buffer 
	depthBufferFormat = chooseSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT }, 
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

	// Create the depth buffer image
	depthBufferImage = createImage(swapChainExtent.width, swapChainExtent.height, depthBufferFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthBufferImageMemory);

	// Create depth buffer image view
	depthBufferImageView = createImageView(depthBufferImage, depthBufferFormat, VK_IMAGE_ASPECT_DEPTH_BIT);


}

void VulkanRenderer::createFrameBuffers()
{
	// Resize frame buffer count to equal swap chain image count (one framebuffer per image so we can draw to multiple at a time)
	swapChainFramebuffers.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {

		std::array<VkImageView, 2> attachments = {
			swapChainImages[i].imageView,
			depthBufferImageView
		};

		VkFramebufferCreateInfo frameBufferCreateInfo = {};
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = renderPass; // Renderpass layout the framebuffer will be used with
		frameBufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		frameBufferCreateInfo.pAttachments = attachments.data(); // List of attachments 1-to-1 with renderpass
		frameBufferCreateInfo.width = swapChainExtent.width; // Framebuffer width
		frameBufferCreateInfo.height = swapChainExtent.height; // Framebuffer height
		frameBufferCreateInfo.layers = 1; // Framebuffer layers

		VkResult result = vkCreateFramebuffer(mainDevice.logicalDevice, &frameBufferCreateInfo, nullptr, &swapChainFramebuffers[i]);
		if (result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a framebuffer");
		}
	}
}

void VulkanRenderer::createCommandPool()
{
	// Get Indicies of queue families from device
	QueueFamilyIndicies queueFamilyIndicies = getQueueFamilies(mainDevice.physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndicies.graphicsFamily; // Queue family type that buffers from this command pool will use
	
	// Create a graphics queue family command pool
	VkResult result = vkCreateCommandPool(mainDevice.logicalDevice, &poolInfo, nullptr, &graphicsCommandPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pool (Command Pool)");
	}
}

void VulkanRenderer::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo cbAllocInfo = {}; // Command buffers already exist so we allocate the command buffer from our pool
	cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbAllocInfo.commandPool = graphicsCommandPool;
	cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // Primary means that you can submit directly to queue, secondary can only be executed by other command buffers vkCmdExecuteCommands
	cbAllocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

	// Allocate command buffers from graphics pool and place handles in array of buffers
	VkResult result = vkAllocateCommandBuffers(mainDevice.logicalDevice, &cbAllocInfo, commandBuffers.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate Command Buffers!");
	}
}

void VulkanRenderer::createSynchronisation()
{
	imageAvailable.resize(MAX_FRAME_DRAWS);
	renderFinished.resize(MAX_FRAME_DRAWS);
	drawFences.resize(MAX_FRAME_DRAWS);

	// Semaphore creation information
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Fence creation information
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAME_DRAWS; i++) {
		if (vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &imageAvailable[i]) != VK_SUCCESS ||
			vkCreateSemaphore(mainDevice.logicalDevice, &semaphoreCreateInfo, nullptr, &renderFinished[i]) != VK_SUCCESS ||
			vkCreateFence(mainDevice.logicalDevice, &fenceCreateInfo, nullptr, &drawFences[i]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create a sempahore and/or Fence!");
		}
	}
}

void VulkanRenderer::createDescriptorSetLayout()
{
	// UboViewProjection binding info
	VkDescriptorSetLayoutBinding vpLayoutBinding = {};
	vpLayoutBinding.binding = 0; // Binding point in shader (designated by binding number in shader)
	vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; // Type of descriptor (uniform, dynamic uniform)
	vpLayoutBinding.descriptorCount = 1;
	vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage to bind to
	vpLayoutBinding.pImmutableSamplers = nullptr; // Not using any textures atm (For texture: can make sampler data immutable by specifying in layout

	VkDescriptorSetLayoutBinding modelLayoutBinding = {};
	modelLayoutBinding.binding = 1;
	modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelLayoutBinding.descriptorCount = 1;
	modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	modelLayoutBinding.pImmutableSamplers = nullptr;

	std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding, modelLayoutBinding };

	// Create Descriptor Set Layout with given bindings
	VkDescriptorSetLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size()); // Number of binding infos
	createInfo.pBindings = layoutBindings.data(); // Pointer to binding info

	VkResult result = vkCreateDescriptorSetLayout(mainDevice.logicalDevice, &createInfo, nullptr, &descriptorSetLayout);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Descriptor Set Layout!");
	}
}

void VulkanRenderer::createPushConstantRange()
{
	// Define push constant values
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // Shader stage push constant will go to
	pushConstantRange.offset = 0; // Offset into given data to pass push constant
	pushConstantRange.size = sizeof(Model);
}

void VulkanRenderer::createUniformBuffers()
{
	// View projection buffer size
	VkDeviceSize vpBufferSize = sizeof(UboViewProjection);
	VkDeviceSize modelBufferSize = modelUniformAlignment * MAX_OBJECTS;

	// One uniform buffer for each image (and by extention, command buffer)
	vpUniformBuffer.resize(swapChainImages.size());
	vpUniformBufferMemory.resize(swapChainImages.size());
	modelDUniformBuffer.resize(swapChainImages.size());
	modelDUniformBufferMemory.resize(swapChainImages.size());

	// Create the uniform buffers
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

		createBuffer(mainDevice.physicalDevice, mainDevice.logicalDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);
	}
}

void VulkanRenderer::createDescriptorPool()
{
	// Type of descriptors + how many descriptors (not descriptor sets) We only have only one descriptor in our shader (VP matrix)
	VkDescriptorPoolSize vpPoolSize = {};
	vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());
	vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	// Model pool (Dynamic)
	VkDescriptorPoolSize modelPoolSize;
	modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size()); 

	std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize, modelPoolSize };

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size()); // Amount of pool sizes being passed
	poolCreateInfo.pPoolSizes = descriptorPoolSizes.data(); // Pool sizes to create pool with
	poolCreateInfo.maxSets = static_cast<uint32_t>(swapChainImages.size()); // Maximum number of descriptor sets that can be created from pool
	
	VkResult result = vkCreateDescriptorPool(mainDevice.logicalDevice, &poolCreateInfo, nullptr, &descriptorPool);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create a Descriptor Pool!");
	}
}

void VulkanRenderer::createDescriptorSets()
{
	descriptorSets.resize(swapChainImages.size()); // Resize descriptor set list so one for every buffer

	std::vector<VkDescriptorSetLayout> setLayouts(swapChainImages.size(), descriptorSetLayout);

	VkDescriptorSetAllocateInfo setAllocInfo = {};
	setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	setAllocInfo.descriptorPool = descriptorPool; // Pool to allocate descriptor set from
	setAllocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()); // Number of sets to allocate
	setAllocInfo.pSetLayouts = setLayouts.data(); // Layouts to use to allocate sets (1:1 relationship)

	// Allocate descriptor sets
	VkResult result = vkAllocateDescriptorSets(mainDevice.logicalDevice, &setAllocInfo, descriptorSets.data());
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate Descriptor Sets");
	}

	// Update all of the descriptor set buffer bindings
	for (size_t i = 0; i < swapChainImages.size(); i++) {

		// VIEW PROJECTION DESCRIPTOR
		// Buffer info and data offset info
		VkDescriptorBufferInfo vpBufferInfo = {};
		vpBufferInfo.buffer = vpUniformBuffer[i]; // Buffer to get data from
		vpBufferInfo.offset = 0; // Position where data starts
		vpBufferInfo.range = sizeof(UboViewProjection); // Size of data 

		// Data about connection between binding and buffer
		VkWriteDescriptorSet vpSetWrite = {};
		vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		vpSetWrite.dstSet = descriptorSets[i]; // Descriptor set to update
		vpSetWrite.dstBinding = 0; // Binding to update (matches with binding on layout/shader)
		vpSetWrite.dstArrayElement = 0; // Index in array to update
		vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		vpSetWrite.descriptorCount = 1;
		vpSetWrite.pBufferInfo = &vpBufferInfo; // Information about buffer data to bind

		// MODEL DESCRIPTOR
		// Model Buffer Binding Info
		VkDescriptorBufferInfo modelBufferBindingInfo = {};
		modelBufferBindingInfo.buffer = modelDUniformBuffer[i];
		modelBufferBindingInfo.offset = 0;
		modelBufferBindingInfo.range = modelUniformAlignment; 

		VkWriteDescriptorSet modelSetWrite = {};
		modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		modelSetWrite.dstSet = descriptorSets[i];
		modelSetWrite.dstBinding = 1;
		modelSetWrite.dstArrayElement = 0;
		modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		modelSetWrite.descriptorCount = 1;
		modelSetWrite.pBufferInfo = &modelBufferBindingInfo;

		std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite, modelSetWrite };

		// Update the descriptor sets with new buffer binding info
		vkUpdateDescriptorSets(mainDevice.logicalDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(), 0, nullptr);
	}
}

void VulkanRenderer::updateUniformBuffers(uint32_t imageIndex)
{
	// Copy VP data
	void* data;
	vkMapMemory(mainDevice.logicalDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
	memcpy(data, &uboViewProjection, sizeof(UboViewProjection));
	vkUnmapMemory(mainDevice.logicalDevice,vpUniformBufferMemory[imageIndex]);

	// Copy Model data
	for (size_t i = 0; i < meshList.size(); i++) {
		Model* thisModel = (Model*)((uint64_t)modelTransferSpace + (i * modelUniformAlignment));
		*thisModel = meshList[i].getModel();
	}
	// Map the list of model data
	vkMapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex], 0, modelUniformAlignment * meshList.size(), 0, &data);
	memcpy(data, modelTransferSpace, modelUniformAlignment * meshList.size());
	vkUnmapMemory(mainDevice.logicalDevice, modelDUniformBufferMemory[imageIndex]);
}

void VulkanRenderer::recordCommands(uint32_t currentImage)
{
	// Information about how to begin each command buffer
	VkCommandBufferBeginInfo bufferBeginInfo = {};
	bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Information about how to begin a render pass (only needed for graphical applications)
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = swapChainExtent;
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f }; // Color attachment clear value
	clearValues[1].depthStencil.depth = 1.0f;

	renderPassBeginInfo.pClearValues = clearValues.data(); // List of clear values
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

	renderPassBeginInfo.framebuffer = swapChainFramebuffers[currentImage];

	VkResult result = vkBeginCommandBuffer(commandBuffers[currentImage], &bufferBeginInfo);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to start recording to a command buffer!");
	}

	vkCmdBeginRenderPass(commandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// Bind Pipeline to be used in renderpass
		vkCmdBindPipeline(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		for (size_t j = 0; j < meshList.size(); j++) {
			// Bind our vertex buffer
			VkBuffer vertexBuffers[] = { meshList[j].getVertexBuffer() }; // Buffers to bind
			VkDeviceSize offsets[] = { 0 }; // Offsets into buffers being bound
			vkCmdBindVertexBuffers(commandBuffers[currentImage], 0, 1, vertexBuffers, offsets); // Command to bind vertex buffer before drawing with them
			vkCmdBindIndexBuffer(commandBuffers[currentImage], meshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32); // Bind mesh index buffer with 0 offset and using uint32 type

			// Dynamic offset amount
			uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

			// Bind descriptor sets
			vkCmdBindDescriptorSets(commandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
				0, 1, &descriptorSets[currentImage], 1, &dynamicOffset);

			vkCmdPushConstants(commandBuffers[currentImage], pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Model), &meshList[j].getModel());

			// Execute our pipeline
			vkCmdDrawIndexed(commandBuffers[currentImage], meshList[j].getIndexCount(), 1, 0, 0, 0);
		}

	vkCmdEndRenderPass(commandBuffers[currentImage]);

	result = vkEndCommandBuffer(commandBuffers[currentImage]);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to stop recording to a command buffer!");
	}
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.image = image; // Image to create view for
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Type of image (1D, 2D, 3D, Cube etc)
	viewCreateInfo.format = format; // Format of image data
	viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY; // Allows remapping of rgba components to other rgba values
	viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	// Subresources allow the view to view only a part of an images
	viewCreateInfo.subresourceRange.aspectMask = aspectFlags; // Which aspect of image to view (e.g. COLOR_BIT for viewing color)
	viewCreateInfo.subresourceRange.baseMipLevel = 0; // Start mipmap level to view from
	viewCreateInfo.subresourceRange.levelCount = 1; // Number of mipmap levels to view
	viewCreateInfo.subresourceRange.baseArrayLayer = 0; // Start array level to view from
	viewCreateInfo.subresourceRange.layerCount = 1; // How many array levels to view

	// Create image view and return it
	VkImageView imageView;
	VkResult result = vkCreateImageView(mainDevice.logicalDevice, &viewCreateInfo, nullptr, &imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create an image view");
	}

	return imageView;
}

VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code)
{ 
	// Shader module creation info
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.codeSize = code.size(); // Size of code
	shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t *>(code.data()); // Pointer to code (uint32_t pointer type)

	VkShaderModule shaderModule;
	VkResult result = vkCreateShaderModule(mainDevice.logicalDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to creata a shader module");
	}

	return shaderModule;
}

VkImage VulkanRenderer::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory)
{
	// CREATE IMAGE
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D; // Type of image (1D, 2D, 3D)
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1; // Depth of image (just 1, no 3D aspect)
	imageCreateInfo.mipLevels = 1; // Number of mipmap levels
	imageCreateInfo.arrayLayers = 1; // Number of levels in image array
	imageCreateInfo.format = format; // Format type of image (depth format, colour etc)
	imageCreateInfo.tiling = tiling; // How image data should be tiled, (arranged for optimal reading)
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Layout of image data on creation
	imageCreateInfo.usage = useFlags; // Being used as a depth buffer attachment (Bit flag defining what image will be used for)
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT; // Number of samples for multisamples
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // Whether image can be shared between queues

	VkImage image;
	VkResult result = vkCreateImage(mainDevice.logicalDevice, &imageCreateInfo, nullptr, &image);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create an image!");
	}

	// CREATE MEMORY FOR IMAGE

	// Get memory requirements for a type of image
	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(mainDevice.logicalDevice, image, &memoryRequirements);

	// Allocate memory using image requirements and user defined properties
	VkMemoryAllocateInfo memoryAllocInfo = {};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memoryRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mainDevice.physicalDevice, memoryRequirements.memoryTypeBits, propFlags);

	result = vkAllocateMemory(mainDevice.logicalDevice, &memoryAllocInfo, nullptr, imageMemory);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate memory for image!");
	}

	// Connect memory to image
	vkBindImageMemory(mainDevice.logicalDevice, image, *imageMemory, 0);

	return image;
}
