#version 450

layout(set = 0, binding = 0) uniform LightingUniform
{
	vec4 lightPos;
	vec4 lightDir;
	vec4 lightColour;
	mat4 LightSpaceMatrix;
}LightUBO;

layout(push_constant) uniform Push
{
	mat4 model;
	int textureIndex;
    bool isTextured;
} push;


layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 tex;
layout(location = 3) in vec3 color;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 WorldNormal;
void main()
{
	gl_Position = LightUBO.LightSpaceMatrix * push.model * vec4(position, 1.0f);
}