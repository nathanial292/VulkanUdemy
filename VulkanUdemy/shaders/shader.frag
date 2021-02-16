#version 450

layout(location = 0) in vec3 fragCol;
layout(location = 1) in vec2 fragTex;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 FragPos;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 outColour; 	// Final output colour (must also have location

// Dynamic buffers
layout(set = 0, binding = 1) uniform UboModel {
	mat4 model;
	bool hasTexture;
} uboModel;

// Also dynamic buffer
layout(set = 0, binding = 2) uniform DirectionalLight {
	vec3 colour;
	vec3 direction;
	float ambientIntensity;
	float diffuseIntensity;
} directionalLight;

vec4 CalcDirectionalLight()
{
	vec4 ambientColour = vec4(directionalLight.colour, 1.0f) * directionalLight.ambientIntensity;
	
	// Normal mapping
	vec3 normal = normalize(Normal);
	vec3 lightDir = normalize(directionalLight.direction - FragPos);
	
	float diffuseFactor = max(dot(normal, lightDir), 0.0f);
	vec4 diffuseColour = vec4(directionalLight.colour * directionalLight.diffuseIntensity * diffuseFactor, 1.0f);
	
	return (ambientColour + diffuseColour);
}

void main() 
{
	vec4 finalColour = CalcDirectionalLight();
		
	if (uboModel.hasTexture) outColour = texture(textureSampler, fragTex) * finalColour;
	else outColour = outColour = vec4(fragCol, 1.0);
}