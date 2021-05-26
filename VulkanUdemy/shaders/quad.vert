#version 450
layout(location = 0) in vec3 in_pos;
layout(location = 2) in vec2 tex;

layout (location = 0) out vec2 outUV;

void main() 
{
	gl_Position = vec4(in_pos.x, in_pos.y, 0.0, 1.0);
	outUV = tex;

	//outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	//gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}