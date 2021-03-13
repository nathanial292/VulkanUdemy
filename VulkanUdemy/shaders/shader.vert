#version 450 		// Use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 tex;
layout(location = 3) in vec3 normal;

layout(set = 0, binding = 0) uniform UboViewProjection {
	mat4 projection;
	mat4 view;
} uboViewProjection;


layout(push_constant) uniform PushModel {
	mat4 model;
} pushModel;

layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragTex;
layout(location = 2) out vec3 Normal;
layout(location = 3) out vec3 FragPos;

void main() {
	gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0);
	
	//Normal = mat3(transpose(inverse(pushModel.model))) * normal;  
	Normal = normal;
	
	FragPos = vec3(pushModel.model * vec4(pos, 1.0)); 	
	fragCol = col;
	fragTex = tex;
}