#version 450

// This shader will behave the same as the fs_quad except it will composite the colours for output
// this is the swapchain present shader
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform SceneUniform
{
	mat4 model;
	mat4 view;
	mat4 projection;

	float fov;
	float nearPlane;
	float farPlane;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D normal;
layout(set = 0, binding = 2) uniform sampler2D depthTex;
layout(set = 0, binding = 3) uniform sampler2D albedo;

layout(set = 0, binding = 4) uniform LightingUniform
{
	vec4 lightPos;
	vec4 lightDir;
	vec4 lightColour;
}LightUBO;

// reconstruct positions using depth returns view space position
vec4 depthToPosition()
{
	float depth = texture(depthTex, uv).x;
	vec4 clipspace = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewspace = inverse(ubo.projection) * clipspace;
	viewspace.xyz /= viewspace.w;
	vec4 worldSpacePos = inverse(ubo.view) * viewspace;

	return worldSpacePos;
}

void main() {
   
   // Make light position, color and direction uniforms
   vec4 lightColour = LightUBO.lightColour;
   vec4 lightPosition = LightUBO.lightPos; // vec4(0.0, 10.0f, 1.0f, 1.0f)
   vec4 lightDirection = LightUBO.lightDir; // vec4(0.0, -1.0, 0.0, 1.0)

   vec3 WorldPos = depthToPosition().xyz;
   vec3 normal = normalize(texture(normal, uv).xyz);
   vec3 color = texture(albedo, uv).xyz;

   float ambientAmount = 0.3;
   vec3 ambient = ambientAmount * lightColour.xyz;

   vec3 lightDir = normalize(lightPosition.xyz - WorldPos);
   float NdotL = max(dot(normal, lightDir), 0.0);
   vec3 diffuse = NdotL * lightColour.xyz;

   vec3 shade = (ambient + diffuse) * color.xyz; // should be object colour

   outColor = vec4(shade, 1.0);
}

