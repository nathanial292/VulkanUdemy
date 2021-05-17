#version 450

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	mat4 lightSpace;
} ubo;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

 
void main()
{
	gl_Position =  ubo.lightSpace * vec4(inPos, 1.0);
}