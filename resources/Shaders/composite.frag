#version 450

// This shader will behave the same as the fs_quad except it will composite the colours for output
// this is the swapchain present shader
layout(location = 0) in vec2 uv;


void main() {

   outColor = vec4(texture(textures[push.textureIndex], uv).xyz, 1.0);
}

