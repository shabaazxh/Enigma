#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform sampler2D normalTex;
layout(set = 0, binding = 2) uniform sampler2D depthTex;


void main() {
	
	vec3 normal = normalize(texture(normalTex, uv).xyz);
    outColor = vec4(normal, 1.0);
}

