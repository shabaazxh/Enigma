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
layout(location = 4) in ivec4 boneIDs;
layout(location = 5) in vec4 weights;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 uv;
layout(location = 2) out vec3 WorldNormal;

layout(set = 2, binding = 0) uniform UBOBones { mat4 boneMatrices[300]; };

void main()
{
    mat4 m = weights[0] * boneMatrices[boneIDs[0]] + weights[1] * boneMatrices[boneIDs[1]] +
        weights[2] * boneMatrices[boneIDs[2]] + weights[3] * boneMatrices[boneIDs[3]];
	gl_Position = LightUBO.LightSpaceMatrix * push.model * m * vec4(position, 1.0f);
}