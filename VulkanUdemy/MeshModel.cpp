#include "MeshModel.h"

namespace vulkan {
	MeshModel::MeshModel(std::vector<Mesh> newMeshList)
	{
		meshList = newMeshList;
		model.model = glm::mat4(1.0f);
		model.inverseModel = glm::mat4(1.0f);
		model.hasTexture = false;
	}

	MeshModel::~MeshModel()
	{
	}

	size_t MeshModel::getMeshCount()
	{
		return meshList.size();
	}

	Mesh* MeshModel::getMesh(size_t index)
	{
		if (index >= meshList.size()) {
			throw std::runtime_error("Attempted to access invalid Mesh index");
		}

		return &meshList[index];
	}

	void MeshModel::setModel(glm::mat4 newModel)
	{
		model.model = newModel;
		model.inverseModel = glm::transpose(glm::inverse(newModel));
	}

	std::vector<std::string> MeshModel::LoadMaterials(const aiScene* scene)
	{
		// Create 1:1 sized list of textures
		std::vector<std::string> textureList(scene->mNumMaterials);

		// Go through each material and copy its texture file name (if it exists)
		for (size_t i = 0; i < scene->mNumMaterials; i++) {
			aiMaterial* material = scene->mMaterials[i]; // Get material

			// Initalise the texture to empty string (will be replaced if texture exists)
			textureList[i] = "";
			// Check for a diffuse texture (standard detail texture)
			if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
				// Get the path of the texture file
				aiString path;
				if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
					// Cut off any directory information already present
					int index = std::string(path.data).rfind("\\"); // Find the position of the last backslash in the string (e.g. C:\users\Ben\Documents\thing.obj
					std::string fileName = std::string(path.data).substr(index + 1); // Get the "thing.obj" (+1 from the backslash)

					textureList[i] = fileName;
				}
			}
		}
		return textureList;
	}

	std::vector<Mesh> MeshModel::LoadNode(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiNode* node, const aiScene* scene, std::vector<int> matToTex)
	{
		std::vector<Mesh> meshList;
		// Go through each mesh at this node and create it, then add it to our meshList
		for (size_t i = 0; i < node->mNumMeshes; i++) {
			meshList.push_back(LoadMesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, scene->mMeshes[node->mMeshes[i]], scene, matToTex));
		}

		// Go through each node attached to this node and load it, then append their meshes to this nodes mesh list
		for (size_t i = 0; i < node->mNumChildren; i++) {
			std::vector<Mesh> newList = LoadNode(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, node->mChildren[i], scene, matToTex);
			meshList.insert(meshList.end(), newList.begin(), newList.end()); // Insert at the end of meshlist, all nodes from the start to end of newList (child node)
		}

		return meshList;
	}

	Mesh MeshModel::LoadMesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice, VkQueue transferQueue, VkCommandPool transferCommandPool, aiMesh* mesh, const aiScene* scene, std::vector<int> matToTex)
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		vertices.resize(mesh->mNumVertices);
		std::cout << vertices[0].normal.x << " " << vertices[0].normal.y << " " << vertices[0].normal.z << ",";
		for (size_t i = 0; i < mesh->mNumVertices; i++) {
			// Set position
			vertices[i].pos = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };

			// Set texture coords if they exist
			if (mesh->mTextureCoords[0]) {
				vertices[i].tex = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
			}
			else {
				vertices[i].tex = { 0.0f, 0.0f };
			}
			vertices[i].col = { 1.0f, 1.0f, 1.0f };
			vertices[i].normal = { mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };

			//std::cout << vertices[i].pos.x << vertices[i].pos.y << vertices[i].pos.z;
			//std::cout << vertices[i].col.x << vertices[i].col.y << vertices[i].col.z;
			//std::cout << vertices[i].tex.x << vertices[i].tex.y;
			//std::cout << vertices[i].normal.x << vertices[i].normal.y << vertices[i].normal.z;
		}

		// Go through faces indicies and add to list
		for (size_t i = 0; i < mesh->mNumFaces; i++) {
			// Get face
			aiFace face = mesh->mFaces[i];
			for (size_t j = 0; j < face.mNumIndices; j++) {
				indices.push_back(face.mIndices[j]);
			}
		}

		// Create new mesh with details and return
		Mesh newMesh = Mesh(newPhysicalDevice, newDevice, transferQueue, transferCommandPool, &indices, &vertices, matToTex[mesh->mMaterialIndex]);

		return newMesh;
	}

	void MeshModel::destroyMeshModel()
	{
		for (auto& mesh : meshList) {
			mesh.destroyBuffers();
		}
	}
}