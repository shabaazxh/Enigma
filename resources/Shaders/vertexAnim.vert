#version 450


layout(set = 0, binding = 0) uniform SceneUniform
{
	mat4 model;
	mat4 view;
	mat4 projection;

	float fov;
	float nearPlane;
	float farPlane;
} ubo;

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
layout(location = 3) out vec4 WorldPosition;

layout(set = 2, binding = 0) uniform UBOBones { mat4 boneMatrices[300]; };

void main()
{
    mat4 m = weights[0] * boneMatrices[boneIDs[0]] + weights[1] * boneMatrices[boneIDs[1]] +
        weights[2] * boneMatrices[boneIDs[2]] + weights[3] * boneMatrices[boneIDs[3]];

    WorldPosition = push.model * m * vec4(position, 1.0);
    gl_Position = ubo.projection * ubo.view * WorldPosition;

    fragColor = color;
    uv = tex;
    WorldNormal = normal;
}