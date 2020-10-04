#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec3 fragCol;
void main() {
	outColor = vec4(fragCol, 1.0);
}