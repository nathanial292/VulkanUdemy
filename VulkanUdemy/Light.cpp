#include "Light.h"

namespace vulkan {
	Light::Light()
	{
		colour = glm::vec3(1.0f, 1.0f, 1.0f);
		ambientIntensity = 1.0f;

		diffuseIntensity = 0.0f;
	}

	Light::Light(glm::vec3 colour, float aIntensity, float dIntensity)
	{
		this->colour = colour;
		ambientIntensity = aIntensity;

		diffuseIntensity = dIntensity;
	}

	Light::~Light()
	{
	}
}