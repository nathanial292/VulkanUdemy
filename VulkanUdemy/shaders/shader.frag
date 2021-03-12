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

// Uniform buffer for light
layout(set = 0, binding = 2) uniform DirectionalLight {
	vec3 colour;
	vec3 direction;
	float ambientIntensity;
	float diffuseIntensity;
} directionalLight;

layout(set = 0, binding = 3) uniform CameraPosition {
	vec3 cameraPos;	
} cameraPosition;

vec4 CalcDirectionalLight()
{
	vec4 ambientColour = vec4(directionalLight.colour, 1.0f) * 0.4;
	
	vec3 normal = normalize(Normal);
	vec3 lightDir = -normalize(directionalLight.direction - FragPos);
	
	float diffuseFactor = max(dot(normal, -lightDir), 0.0f);
	vec4 diffuseColour = vec4(directionalLight.colour * 1.0 * diffuseFactor, 1.0f);
	
	vec4 specularColour = vec4(0, 0, 0, 0);
	if(diffuseFactor > 0.0f)
	{
		vec3 fragToEye = normalize(cameraPosition.cameraPos - FragPos);
		vec3 reflectedVertex = normalize(reflect(lightDir, normal));
		
		float specularFactor = dot(fragToEye, reflectedVertex);
		if(specularFactor > 0.0f)
		{
			specularFactor = pow(specularFactor, 128);
			specularColour = vec4(directionalLight.colour * 0.5 * specularFactor, 1.0f);
		}
	}
	
	return (ambientColour + diffuseColour + specularColour);
}

void main() 
{
	vec4 finalColour = CalcDirectionalLight();		
	if (uboModel.hasTexture) {
		outColour = texture(textureSampler, fragTex) * finalColour;
		
		//vec3 normal = normalize(Normal);
		//vec3 lightDir = normalize(directionalLight.direction - FragPos);
		
		//float diffuseFactor = max(dot(normal, lightDir), 0.0f);
		
		//outColour = vec4(diffuseFactor, 1.0, 1.0, 1.0f);
		//float bug=0.0;

		//if(directionalLight.direction.y <= 0) bug=1.0;

		//outColour.x+=bug;
	}
	else {
		outColour = vec4(fragCol, 1.0);
	}
}