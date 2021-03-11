#include "DirectionalLight.h"

namespace vulkan {
	DirectionalLight::DirectionalLight() : Light()
	{
		direction = glm::vec3(0.0f, -1.0f, 0.0f);
	}

	DirectionalLight::DirectionalLight(float red, float green, float blue,
		float aIntensity, float dIntensity,
		float xDir, float yDir, float zDir) : Light(red, green, blue, aIntensity, dIntensity)
	{
		direction = glm::vec3(xDir, yDir, zDir);
	}

	UniformLight DirectionalLight::getLight()
	{
		UniformLight light = {};
		light.colour = colour;
		light.direction = direction;
		light.ambientIntensity = ambientIntensity;
		light.diffuseIntensity = diffuseIntensity;

		return light;
	}

	void DirectionalLight::UpdatePosition(float xDir, float yDir, float zDir)
	{
		direction = glm::vec3(xDir, yDir, zDir);
	}

	DirectionalLight::~DirectionalLight()
	{
	}
}