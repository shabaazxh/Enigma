#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 WorldNormal;

layout(location = 0) out vec4 gNormal;
//layout(location = 1) out float depth;

layout(set = 0, binding = 0) uniform SceneUniform
{
	mat4 model;
	mat4 view;
	mat4 projection;

	float fov;
	float nearPlane;
	float farPlane;
} ubo;

layout(set = 1, binding = 0) uniform sampler2D textures[40];

layout(push_constant) uniform Push
{
	mat4 model;
	int textureIndex;
    bool isTextured;
} push;


void main() {

   //gNormal = vec4(vec3(1.0, 0.3, 0.6), 1.0);
   gNormal = normalize(vec4(WorldNormal, 1.0));
   //depth = gl_FragCoord.z;
}

