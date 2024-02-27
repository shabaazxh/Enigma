#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform Push
{
	mat4 model;
	int textureIndex;
} push;

// array size needs to be known at compile time so 40 is set for default for now
// this can fail if a model has textures > 40
layout(set = 1, binding = 0) uniform sampler2D textures[40];



void main()
{
	outColor = vec4(texture(textures[push.textureIndex], uv).xyz, 1.0f);
}