#version 450

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;

layout(binding=0, set=0) uniform UboProjectionModel {
	mat4 projection;
	mat4 view;
} uboProjectionModel;

layout(binding = 1, set=0) uniform UboModel {
	mat4 model;
} uboModel;

layout(location = 0) out vec3 fragCol;

void main() {
	gl_Position = uboProjectionModel.projection * uboProjectionModel.view * uboModel.model * vec4(pos, 1.0);
	fragCol = col;
}