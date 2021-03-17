#pragma once

#include <glm/glm.hpp>
#include "Mesh.h"
#include <vector>
#include <assimp/scene.h>

namespace vulkan {
	class MeshModel {
	public:
		MeshModel(std::vector<Mesh> newMeshList);
		~MeshModel();

		size_t getMeshCount();
		Mesh* getMesh(size_t index);

		Model getModel() { return model; }
		void setModel(glm::mat4 newModel);
		void setTexture(bool state)
		{
			model.hasTexture = state;
		}

		static std::vector<std::string> LoadMaterials(const aiScene* scene);
		static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex);
		static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex);
		void destroyMeshModel();

	private:
		std::vector<Mesh> meshList;
		Model model;
	};
}