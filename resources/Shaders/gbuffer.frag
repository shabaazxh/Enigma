#version 450

layout(location = 0) in vec3 color;
layout(location = 1) in vec2 uv;
layout(location = 3) in vec3 WorldNormal;

layout(location = 0) out vec3 gNormal;


layout(set = 1, binding = 0) uniform sampler2D textures[40];

void main() {

   gNormal = WorldNormal;
}

