#version 450
layout(location = 0) in vec2 position;

layout(location = 0) out vec2 vsTexcood;

layout(push_constant) uniform Push { mat4 transformMatrix; }
push;

const vec2[4] texcoord = vec2[](vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1));

void main() {
    gl_Position = push.transformMatrix * vec4(position, 0, 1.0f);
    vsTexcood = texcoord[gl_VertexIndex];
}