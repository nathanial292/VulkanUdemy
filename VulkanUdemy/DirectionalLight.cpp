#include "DirectionalLight.h"

namespace vulkan {
	DirectionalLight::DirectionalLight() : Light()
	{
		direction = glm::vec3(0.0f, -1.0f, 0.0f);
	}

	DirectionalLight::DirectionalLight(glm::vec3 position, glm::vec3 colour, float aIntensity, float dIntensity) : Light(colour, aIntensity, dIntensity)
	{
		direction = position;
	}

	UniformLight DirectionalLight::getLight()
	{
		UniformLight light = {};
		light.colour = colour;
		light.ambientIntensity = ambientIntensity;
		light.diffuseIntensity = diffuseIntensity;
		light.direction = direction;

		return light;
	}

	void DirectionalLight::updateLight(glm::vec3* position, glm::vec3* colour, float* ambientIntensity, float* diffuseIntensity)
	{
		this->direction = *position;
		this->colour = *colour;
		this->diffuseIntensity = *diffuseIntensity;
		this->ambientIntensity = *ambientIntensity;
	}

	DirectionalLight::~DirectionalLight()
	{
	}
}