#include "DirectionalLight.h"

namespace vulkan {
	DirectionalLight::DirectionalLight() : Light()
	{
		direction = glm::vec3(0.0f, 0.0f, 0.0f);
		//lightProj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 20.0f);
		lightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	}

	DirectionalLight::DirectionalLight(glm::vec3 position, glm::vec3 colour, float aIntensity, float dIntensity) : Light(colour, aIntensity, dIntensity)
	{
		direction = position;
		//lightProj = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 0.1f, 100.0f);
		lightProj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
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

	glm::vec3 DirectionalLight::getPosition()
	{
		return direction;
	}

	glm::mat4 DirectionalLight::CalculateLightTransform()
	{
		return lightProj * glm::lookAt(direction, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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