#pragma once

#define GLFW_INCLUDE_VULKAN

#include "../../API Wrapper/Window.h"
#include "../../API Wrapper/Camera.h"

#include "Utilities.h"
#include "Mesh.h"
#include "MeshModel.h"
#include "DirectionalLight.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <stdexcept>
#include <vector>
#include <iostream>
#include <set>
#include <algorithm>
#include <array>

//#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

// Shadowmap properties
#define SHADOWMAP_DIM 8192
#define DEFAULT_SHADOWMAP_FILTER VK_FILTER_LINEAR

namespace vulkan {
	class VulkanRenderer
	{
	public:
		bool displayShadowQuad = false;

		VulkanRenderer();

		int init(Window* window, Camera* camera, int sampleCount);

		void updateModel(int modelId, glm::mat4 *newModel);
		void updateModelMesh(int modelId, glm::mat4 *newModel);

		void draw(glm::mat4 projection, glm::mat4 viewMatrix);
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
		void createDepthStencil();
		void createRenderPass();
		void createOffscreenRenderPass();
		void createImguiRenderPass();
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
		void createDescriptorSets();
		void createTextureSampler();

		void createImguiContext();

		// Recreate functions
		void recreateSwapChain();

		// Record functions
		void recordCommands(uint32_t currentImage);
		void drawScene(uint32_t currentImage, bool isOffscreen);
		void updateUniformBuffers(uint32_t imageIndex);

		// Get Functions
		void getPhysicalDevice(int sampleCount);

		// Set functions
		void setMeshList(std::vector<Mesh>* theMeshlist) {
			meshList = *theMeshlist;
		}
		void setModelList(std::vector<MeshModel>* theModelList) {
			modelList = *theModelList;
		}
		void setDirectionalLight(DirectionalLight* light) {
			directionalLight = light;
		}
		void setMultiSampleLevel(int level)
		{
			if (level % 2 == 0 || level == 1) {
				std::cout << level;
				if (level == 1) msaaSamples = VK_SAMPLE_COUNT_1_BIT;
				if (level == 2) msaaSamples = VK_SAMPLE_COUNT_2_BIT;
				if (level == 4) msaaSamples = VK_SAMPLE_COUNT_4_BIT;
				if (level == 8) msaaSamples = VK_SAMPLE_COUNT_8_BIT;

				//recreateSwapChain();
			}
		}
		VkSampleCountFlagBits translateSampleIntToEnum(int level)
		{
			VkSampleCountFlagBits temp;
			if (level % 2 == 0 || level == 1) {
				std::cout << level;
				if (level == 1) temp = VK_SAMPLE_COUNT_1_BIT;
				if (level == 2) temp = VK_SAMPLE_COUNT_2_BIT;
				if (level == 4) temp = VK_SAMPLE_COUNT_4_BIT;
				if (level == 8) temp = VK_SAMPLE_COUNT_8_BIT;
			}
			return temp;
		}
		void setTextureStateModel(int modelIndex, bool state)
		{
			if (modelIndex > modelList.size() - 1) return;
			modelList[modelIndex].setTexture(state);
			int meshCount = modelList[modelIndex].getMeshCount();
			for (int i = 0; i < meshCount; i++) modelList[modelIndex].getMesh(i)->setTexture(state);
		}
		void setTextureStateMesh(int meshIndex, bool state)
		{
			if (meshIndex > meshList.size() - 1) return;
			meshList[meshIndex].setTexture(state);
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

		VkExtent2D getSwapChainExtent() { return swapChainExtent; }

		// Choose Functions
		VkSurfaceFormatKHR chooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		VkPresentModeKHR chooseBestPresentationMode(const std::vector<VkPresentModeKHR>& presentationModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
		VkFormat chooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);

		// Create Functions
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		VkShaderModule createShaderModule(const std::vector<char>& code);
		VkImage createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory* imageMemory, VkSampleCountFlagBits numSamples);

		// Texture creates
		int createTextureImage(std::string fileName);
		int createTexture(std::string fileName);
		int createTextureDescriptor(VkImageView textureImage);

		// Model creation
		MeshModel createMeshModel(std::string modelFile, int texId);
		int createModel(const char* modelName);
		int createModel(const char* modelName, const char* textureName);
		int createMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices, const char* fileName);
		int createMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
		// Colour Resources
		void createColourImage();
		// Lights
		void createDirectionalLight(glm::vec3 position, glm::vec3 colour, float ambientIntensity, float diffuseIntensity);
		void updateDirectionalLight(glm::vec3 *position, glm::vec3 *colour, float *ambientIntensity, float *diffuseIntensity);

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
		std::vector<VkFramebuffer> imguiFrameBuffers;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkCommandBuffer> imguiCommandBuffers;

		// Depth buffer class members
		VkFormat depthBufferFormat;

		// Depth stencil (Full width/height)
		VkImage depthStencilImage;
		VkDeviceMemory depthStencilImageMemory;
		VkImageView depthStencilImageView;

		// Multisample class members
		VkImage colourImage;
		VkDeviceMemory colourImageMemory;
		VkImageView colourImageView;

		VkSampler textureSampler;

		// Assets
		std::vector<VkImage> textureImages;
		std::vector<VkDeviceMemory> textureImageMemory;
		std::vector<VkImageView> textureImageViews;

		// Scene objects
		std::vector<MeshModel> modelList;
		std::vector<Mesh> meshList;

		// Multisample count
		VkSampleCountFlagBits msaaSamples;

		struct {
			VkPipeline scene;
			VkPipeline debug;
			VkPipeline offscreen;
		} pipelines;

		struct FrameBufferAttachment {
			VkImage image;
			VkDeviceMemory mem;
			VkImageView view;
		};

		struct OffscreenPass {
			int32_t width, height;
			VkFramebuffer frameBuffer;
			FrameBufferAttachment depth;
			VkRenderPass renderPass;
			VkSampler depthSampler;
		} offscreenPass;

		// Pipeline
		VkPipelineLayout pipelineLayout;
		VkPipelineLayout debugLayout;
		VkPipelineLayout offscreenPipelineLayout;
		VkRenderPass renderPass;
		VkRenderPass imguiRenderPass;

		// Pools
		VkCommandPool graphicsCommandPool;
		VkCommandPool imguiCommandPool;

		// Utility
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;

		// Scene Settings
		struct UboViewProjection {
			glm::mat4 projection;
			glm::mat4 view;
			glm::mat4 lightTransform;
		} uboViewProjection;

		// Descriptors
		VkDescriptorSetLayout descriptorSetLayout;
		VkDescriptorSetLayout samplerSetLayout;
		VkDescriptorSetLayout shadowSamplerSetLayout;

		VkPushConstantRange pushConstantRange;

		VkDescriptorPool descriptorPool;
		VkDescriptorPool samplerDescriptorPool;
		VkDescriptorPool imguiDescriptorPool;
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

		struct ModelTest {
			glm::mat4 model;
			glm::mat4 inverseModel;
			bool hasTexture;
			glm::mat3 a;
			glm::vec4 c;
			glm::vec4 d;
		};
		Model* modelTransferSpace;

		std::vector<VkBuffer> directionalLightUniformBuffer;
		std::vector<VkDeviceMemory> directionalLightUniformBufferMemory;
		DirectionalLight* directionalLight;

		std::vector<VkBuffer> cameraPositionUniformBuffer;
		std::vector<VkDeviceMemory> cameraPositionUniformBufferMemory;

		// Synchronisation
		std::vector<VkSemaphore> imageAvailable;
		std::vector<VkSemaphore> renderFinished;
		std::vector<VkFence> drawFences;
	};
}