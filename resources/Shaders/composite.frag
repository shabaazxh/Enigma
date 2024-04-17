#version 450

// This shader will behave the same as the fs_quad except it will composite the colours for output
// this is the swapchain present shader
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 1) uniform sampler2D lighting;



void main() {

   outColor = vec4(texture(lighting, uv).xyz, 1.0);
}

