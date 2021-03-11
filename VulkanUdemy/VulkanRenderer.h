#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include "Utilities.h"
#include "Mesh.h"
#include "MeshModel.h"
#include "Window.h"
#include "Camera.h"
#include "DirectionalLight.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include <array>

class VulkanRenderer
{
public:
	VulkanRenderer();

	int init(Window* window, Camera* camera);

	void updateModel(int modelId, glm::mat4 newModel);
	void updateModelMesh(int modelId, glm::mat4 newModel);

	void draw();
	void cleanup();
	void cleanupSwapChain();

	VulkanDevice getVulkanDevice() { return mainDevice; }
	VkQueue getGraphicsQueue() { return graphicsQueue; }
	VkCommandPool getGraphicsCommandPool() { return graphicsCommandPool; }

	// Vulkan Functions
	void createInstance();
	void createLogicalDevice();
	void setupDebugMessenger();
	void createSurface();
	void createSwapChain();
	void createDepthBufferImage();
	void createRenderPass(VkRenderPass* theRenderPass);
	void createDescriptorSetLayout();
	void createPushConstantRange();
	void createGraphicsPipeline();
	void createFrameBuffers();
	void createOffscreenFrameBuffer();
	void createCommandPool();
	void createCommandBuffers();
	void createSynchronisation();
	void createUniformBuffers();
	void createDescriptorPool();
	void createShadowSamplerDescriptorSet();
	void createDescriptorSets();
	void createTextureSampler();

	// Recreate functions
	void recreateSwapChain();

	// Record functions
	void recordCommands(uint32_t currentImage);
	void updateUniformBuffers(uint32_t imageIndex);

	// Get Functions
	void getPhysicalDevice();

	// Set functions
	void setMeshList(std::vector<Mesh>* theMeshlist) {
		meshList = *theMeshlist;
	}
	void setModelList(std::vector<MeshModel>* theModelList) { 
		modelList = *theModelList;
	}
	void setDirectionalLight(DirectionalLight light) {
		directionalLight = light;
	}

	// Allocate Functions
	void allocateDynamicBufferTransferSpace();

	// SUPPORT FUNCTIONS //
	// Checker Functions
	bool checkInstanceExtensionSupport(std::vector<const char*>* checkExtensions);
	bool checkDeviceSuitable(VkPhysicalDevice device);
	bool checkValidiationLayerSupport();
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	// Getter Functions
	QueueFamilyIndicies getQueueFamilies(VkPhysicalDevice device);
	SwapChainDetails getSwapChainDetails(VkPhysicalDevice device);
	VkSampleCountFlagBits getMaxUseableSampleCount();

	// Choose Functions
	VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
	VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
	VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

	// Create Functions
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	VkShaderModule createShaderModule(const std::vector<char>& code);
	VkImage createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory, VkSampleCountFlagBits numSamples);

	int createTextureImage(std::string fileName);
	int createTexture(std::string fileName);
	int createTextureDescriptor(VkImageView textureImage);

	// Model creation
	MeshModel createMeshModel(std::string modelFile, int texId);

	// Colour Resources
	void createColourImage();

	// Loader Functions
	stbi_uc* loadTextureFile(std::string fileName, int* width, int* height, VkDeviceSize* imageSize); // Return a unsigned char byte array

	~VulkanRenderer();

private:
	Window* window;
	Camera* camera;

	uint32_t mipLevels;

	int currentFrame = 0;
	bool frameBufferResized = false;

	// Vulkan Components
	VkInstance instance;

	// Debug Handler
	VkDebugUtilsMessengerEXT debugMessenger;
	VkResult createDebugMessenger(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator);
	void destroyDebugMessenger(VkAllocationCallbacks* pAllocator);
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	// MAIN FUNCTIONS
	VulkanDevice mainDevice;

	VkQueue graphicsQueue;
	VkQueue presentationQueue;
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	std::vector<SwapChainImage> swapChainImages;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkCommandBuffer> commandBuffers;

	// Depth buffer class members
	VkImage depthBufferImage;
	VkDeviceMemory depthBufferImageMemory;
	VkImageView depthBufferImageView;
	VkFormat depthBufferFormat;

	// Multisample class members
	VkImage colourImage;
	VkDeviceMemory colourImageMemory;
	VkImageView colourImageView;
	// Multisample count
	VkSampleCountFlagBits msaaSamples;

	VkSampler textureSampler;

	// Assets
	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// Scene objects
	std::vector<MeshModel> modelList;
	std::vector<Mesh> meshList;

	// Pipeline
	VkPipelineLayout pipelineLayout;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;


	// Shadowmap
	uint32_t shadowWidth = 2048;
	uint32_t shadowHeight = 2048;
	// Offscreen Pipeline (for shadows)
	VkFramebuffer offscreenFrameBuffer;
	VkImage offscreenDepthImage;
	VkDeviceMemory offscreenDepthImageMemory;
	VkImageView offscreenDepthImageView;
	VkRenderPass offscreenRenderPass;
	VkSampler offscreenDepthSampler;
	VkDescriptorImageInfo offscreenDescriptor;

	// Pools
	VkCommandPool graphicsCommandPool;

	// Utility
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	// Scene Settings
	struct UboViewProjection {
		glm::mat4 projection;
		glm::mat4 view;
	} uboViewProjection;

	// Descriptors
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSetLayout samplerSetLayout;
	VkDescriptorSetLayout shadowSamplerSetLayout;

	VkPushConstantRange pushConstantRange;

	VkDescriptorPool descriptorPool;
	VkDescriptorPool samplerDescriptorPool;
	VkDescriptorPool shadowSamplerDescriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> samplerDescriptorSets;
	VkDescriptorSet shadowSamplerDescriptorSet;

	// Uniform Buffers (Static for every model)
	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;
	
	// Dynamic uniform buffers (Changes between each mesh)
	VkDeviceSize minUniformBufferOffset;

	std::vector<VkBuffer> modelDUniformBuffer;
	std::vector<VkDeviceMemory> modelDUniformBufferMemory;
	size_t modelUniformAlignment;
	Model* modelTransferSpace;

	std::vector<VkBuffer> directionalLightUniformBuffer;
	std::vector<VkDeviceMemory> directionalLightUniformBufferMemory;
	DirectionalLight directionalLight;

	std::vector<VkBuffer> cameraPositionUniformBuffer;
	std::vector<VkDeviceMemory> cameraPositionUniformBufferMemory;

	// Synchronisation
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;
};

