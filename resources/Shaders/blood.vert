#version 450
layout(location = 0) in vec2 position;

layout(push_constant) uniform Push { mat4 transformMatrix; }
push;

void main() { gl_Position = push.transformMatrix * vec4(position, 0, 1.0f); }