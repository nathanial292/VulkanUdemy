#include <glm/glm.hpp>
#include "Mesh.h"
#include <vector>

#include <assimp/scene.h>

#pragma once
class MeshModel {
public:
	MeshModel(std::vector<Mesh> newMeshList);
	~MeshModel();

	size_t getMeshCount();
	Mesh* getMesh(size_t index);

	glm::mat4 getModel();
	void setModle(glm::mat4 newModel);

	static std::vector<std::string> LoadMaterials(const aiScene* scene);
	static std::vector<Mesh> LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex);
	static Mesh LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex);
	void destroyMeshModel();

private:
	std::vector<Mesh> meshList;
	glm::mat4 model;
};