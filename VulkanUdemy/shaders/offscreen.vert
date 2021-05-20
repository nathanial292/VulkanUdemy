#version 450

layout (location = 0) in vec3 pos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 lightSpace;
} ubo;

layout(push_constant) uniform PushModel {
	mat4 model;
	mat4 inverseModel;
	bool hasTexture;
} pushModel;
 
void main()
{
	gl_Position =  ubo.lightSpace * pushModel.model * vec4(pos, 1.0);
}