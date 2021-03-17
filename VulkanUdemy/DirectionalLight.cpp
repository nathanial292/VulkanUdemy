#include "DirectionalLight.h"

namespace vulkan {
	DirectionalLight::DirectionalLight() : Light()
	{
		direction = glm::vec3(0.0f, 0.0f, 0.0f);
	}

	DirectionalLight::DirectionalLight(glm::vec3 position, glm::vec3 colour, float aIntensity, float dIntensity) : Light(colour, aIntensity, dIntensity)
	{
		direction = position;
	}

	UniformLight DirectionalLight::getLight()
	{
		UniformLight light = {};
		light.direction = this->direction;
		light.colour = this->colour;
		light.ambientIntensity = this->ambientIntensity;
		light.diffuseIntensity = this->diffuseIntensity;
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