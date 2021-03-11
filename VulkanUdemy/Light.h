#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace vulkan {
	class Light
	{
	public:
		Light();
		Light(float red, float green, float blue, float aIntensity, float dIntensity);

		~Light();
	protected:
		glm::vec3 colour;
		float ambientIntensity;
		float diffuseIntensity;

		glm::mat4 lightProj;
	};
}