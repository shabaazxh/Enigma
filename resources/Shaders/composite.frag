#version 450

// This shader will behave the same as the fs_quad except it will composite the colours for output
// this is the swapchain present shader
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 outColor;


layout(set = 0, binding = 1) uniform sampler2D lighting;

// sRGB linear -> compressed
vec3 Apply_sRGB_OETF(vec3 linear)
{
    bvec3 cutoff = lessThan(linear, vec3(0.0031308));
    vec3 higher = vec3(1.055) * pow(linear, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3 lower = linear * vec3(12.92);

    return mix(higher, lower, cutoff);
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESToneMappingFilm(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main() {

   vec3 lighting_pass = texture(lighting, uv).xyz;

   lighting_pass = ACESToneMappingFilm(lighting_pass);

   lighting_pass = pow(lighting_pass, vec3(1.0 / 2.2));

   vec3 non_linear_output = Apply_sRGB_OETF(lighting_pass);

   outColor = vec4(non_linear_output, 1.0);
}

