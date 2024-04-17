#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec3 WorldNormal;

layout(location = 0) out vec4 gNormal;
layout(location = 1) out vec4 albedo;

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


// for this pass, we don't output to depth
// graphics pipeline will write to depth during depth testing 
// just make sure its transitioned to shader read only optimal 

void main() {
	
   gNormal = normalize(vec4(WorldNormal, 1.0));
   albedo = vec4(texture(textures[push.textureIndex], uv).xyz, 1.0);
}

