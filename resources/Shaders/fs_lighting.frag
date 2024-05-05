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
	vec3 cameraPosition;

	float fov;
	float nearPlane;
	float farPlane;
} ubo;

layout(push_constant) uniform Push
{
	mat4 model;
	int textureIndex;
    bool isTextured;
} push;

layout(set = 0, binding = 1) uniform sampler2D gBuffPosition;
layout(set = 0, binding = 2) uniform sampler2D gBuffNormal;
layout(set = 0, binding = 3) uniform sampler2D depthTex;
layout(set = 0, binding = 4) uniform sampler2D albedo;
layout(set = 0, binding = 5) uniform sampler2D shadowMap;

layout(set = 0, binding = 6) uniform LightingUniform
{
	vec4 lightPos;
	vec4 lightDir;
	vec4 lightColour;
	mat4 LightSpaceMatrix;
}LightUBO;

layout(set = 0, binding = 7) uniform Debug
{
	int debugRenderTarget;
	int stepCount;
	float thickness;
	float maxDistance;
}debugRenderer;

// reconstruct positions using depth returns view space position
vec3 depthToPosition()
{
	float depth = texture(depthTex, uv).x;
	vec4 clipspace = vec4(uv * 2.0 - 1.0, depth, 1.0);
	vec4 viewspace = inverse(ubo.projection) * clipspace;
	vec4 worldSpacePos = inverse(ubo.view) * viewspace;

	return worldSpacePos.xyz /= worldSpacePos.w;
}

vec2 POISSON32[] = vec2[32](
	vec2(0.2981409,0.0490049), vec2(0.1629048,-0.1408463), vec2(0.1691782,-0.3703386),
	vec2(0.3708196,0.2632940), vec2(-0.0602839,-0.2213077), vec2(0.3062163,-0.1364151),
	vec2(0.0094440,-0.0299901), vec2(-0.0753952,-0.3944479), vec2(-0.2073224,-0.3717136),
	vec2(0.1254510,0.0428502), vec2(0.2816537,-0.3045711), vec2(-0.2343018,-0.2459390),
	vec2(0.0625516,-0.2719784), vec2(-0.3949863,-0.2474681), vec2(0.0501389,-0.4359268),
	vec2(-0.1602987,-0.0242505), vec2(0.3998221,0.1279425), vec2(0.1698757,0.2820195),
	vec2(0.4191946,-0.0148812), vec2(0.4103152,-0.2532885), vec2(-0.0010199,0.3389769),
	vec2(-0.2646317,-0.1102498), vec2(0.2064117,0.4451604), vec2(-0.0788299,0.1059370),
	vec2(-0.3209068,0.1344933), vec2(0.0868388,0.1710649), vec2(-0.3878541,-0.0204674),
	vec2(-0.4418672,0.1825800), vec2(-0.3623412,0.3157248), vec2(-0.1956292,0.2076620),
	vec2(0.0205688,0.4664732), vec2(-0.1860556,0.4323920)
);


//
//vec3 test_shadow(vec3 WorldPosition)
//{
//	vec4 fragPosLightSpace = LightUBO.LightSpaceMatrix * vec4(WorldPosition.xyz, 1.0);
//	vec4 projCoords = fragPosLightSpace / fragPosLightSpace.w;
//
//	projCoords.xy = projCoords.xy * 0.5 + 0.5;
//
//	float closestDepth = texture(shadowMap, projCoords.xy).r;
//	vec3 color = texture(albedo, uv).xyz;
//
//	vec3 result = color * closestDepth;
//
//	return result;
//}

bool check(vec2 projectCoords)
{
    if(abs(projectCoords.x) > 0.0 && abs(projectCoords.x) < 1.0  
    && abs(projectCoords.y) > 0.0 && abs(projectCoords.y) < 1.0)
    {
        // we're in screen-space
        return true;
    }
    // coordinate is outside of screen-space
    return false;
}

float LinearizeDepth(float d)
{
	return (2.0 * ubo.nearPlane) / (ubo.farPlane + ubo.nearPlane - d * (ubo.farPlane - ubo.nearPlane));
}


// @LightDirection: World position fragment to light position 
float screen_space_shadows(vec3 LightDirection)
{
	uint  max_steps = debugRenderer.stepCount; // 32
	float max_dist = debugRenderer.maxDistance; // 0.5
	float thickness = 0.05; // 0.05
	float step_length = debugRenderer.thickness; // 0.25

	vec3 ray_pos = (ubo.view * vec4(texture(gBuffPosition, uv).xyz, 1.0)).xyz;
	vec3 ray_dir = normalize((ubo.view * vec4(LightDirection, 1.0)).xyz);

	vec3 ray_step = ray_dir * step_length;

	float occlusion = 0.0;
	vec2 ray_uv = vec2(0.0);

	for(uint i = 0; i < max_steps; i++)
	{
		// step the ray
		ray_pos += ray_step;
		vec4 vpPos = ubo.projection * vec4(ray_pos, 1.0);
		vec2 ray_uv = vpPos.xy / vpPos.w;
		ray_uv = ray_uv * 0.5 + 0.5;

		float ray_depth = vpPos.z / vpPos.w;
		float depth = LinearizeDepth(texture(depthTex, ray_uv).r);

		bool can_camera_see_ray = (ray_depth - depth > 0) && ray_depth - depth < thickness;
		
		if(can_camera_see_ray)
		{
			occlusion = 1.0;
			break;
		}
	}

	return 1.0 - occlusion;
}


vec4 screen_space_reflections()
{
	//float max_distance = debugRenderer.maxDistance;
	int step_count = debugRenderer.stepCount;
	float step_size = debugRenderer.maxDistance;
	float thickness = debugRenderer.thickness;
	
	vec3 world_normal = normalize(texture(gBuffNormal, uv).xyz);
	vec4 WorldPosition = texture(gBuffPosition, uv);

	vec3 cam_dir = normalize(WorldPosition.xyz - ubo.cameraPosition);
	vec4 ray_start = WorldPosition;

	vec3 ray_direction = normalize(reflect(cam_dir.xyz, world_normal));
	vec3 ray_step = ray_direction * step_size;
	
	vec4 project_coords;
	vec3 ray_position = ray_start.xyz += ray_step;
	for(int i = 0; i < step_count; i++)
	{
		project_coords = ubo.projection * ubo.view * vec4(ray_position, 1.0);
		project_coords.xy /= project_coords.w;

		project_coords.xy = project_coords.xy * 0.5 + 0.5;

		float ray_depth = project_coords.z / project_coords.w;

		float depth = (texture(depthTex, project_coords.xy).x);

		if((ray_depth - depth > 0) && ray_depth - depth < thickness)
		{
			//float a = 0.3 * pow(min(1.0, (step_size * step_count / 2) / length(project_coords.xyz)), 2.0);
			//return (1.0 - a) + vec4(texture(albedo, project_coords.xy).xyz, 1.0) * a;
			return texture(albedo, project_coords.xy);
			break;
		}

		ray_position.xyz += ray_step;
	}

	return vec4(0);
}

#define PI 3.14159265359

vec3 fresnel_schlick_approx(float cos_theta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

float normal_distribution(vec3 normal, vec3 halfVec, float r)
{
	float a = r * r;
	float a2 = a * a;
	float n_dot_h = max(dot(normal, halfVec), 0.0);
	float n_dot_h2 = n_dot_h * n_dot_h;

	float num = a2;
	float denom = (n_dot_h2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float geometry_schlick(float n_dot_v, float r)
{
	float roughness = (r + 1.0);
	float k = (roughness * roughness) / 8.0; // for non-IBL

	float num = n_dot_v;
	float denom = n_dot_v * (1.0 - k) + k;

	return num / denom;
}

float geometry_smith(vec3 N, vec3 V, vec3 L, float k)
{
	float n_dot_v = max(dot(N, V), 0.0);
	float n_dot_l = max(dot(N, L), 0.0);
	
	float ggx2 = geometry_schlick(n_dot_v, k);
	float ggx1 = geometry_schlick(n_dot_l, k);

	return ggx1 * ggx2;
}

float PCF(vec3 WorldPosition)
{
	vec4 fragPosLightSpace = LightUBO.LightSpaceMatrix * vec4(WorldPosition.xyz, 1.0);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	float sum = 0.0;
	float filterRadius = 0.007;

	for(int i = 0; i < 16; i++)
	{
		vec2 offset = POISSON32[i];

		vec3 sampleCoords = projCoords + vec3(offset * filterRadius, 0);

		float depthFromMap = texture(shadowMap, sampleCoords.xy).r;
		float currentDepth = sampleCoords.z;
		sum += currentDepth < depthFromMap ? 1.0 : 0.0;
	}

	return sum / 16.0;
}

float Shadow(vec3 WorldPosition)
{
	vec4 fragPosLightSpace = LightUBO.LightSpaceMatrix * vec4(WorldPosition.xyz, 1.0);
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	projCoords.xy = projCoords.xy * 0.5 + 0.5;

	float closestDepth = texture(shadowMap, projCoords.xy).x;

	float currentDepth = projCoords.z;

	float bias = 0.005;
	float shadow = currentDepth > closestDepth + bias? 1.0 : 0.0;

	return shadow;
}

void main() {
   
   // Make light position, color and direction uniforms
   vec4 lightColour = LightUBO.lightColour;
   vec4 lightPosition = LightUBO.lightPos; // vec4(0.0, 10.0f, 1.0f, 1.0f)
   vec4 lightDirection = LightUBO.lightDir; // vec4(0.0, -1.0, 0.0, 1.0)

   vec3 WorldPos = texture(gBuffPosition, uv).xyz;
   vec3 normal = normalize(texture(gBuffNormal, uv).xyz);
   vec3 color = texture(albedo, uv).xyz;

   float ambientAmount = 0.2;
   vec3 ambient = vec3(ambientAmount) * color;

   vec3 lightDir = normalize(lightPosition.xyz - WorldPos); // direction pointing towards the light source 
   float NdotL = max(dot(normal, lightDir), 0.0);
   vec3 diffuse = NdotL * lightColour.xyz;

   float specular_amount = 0.5;
   vec3 view_dir = normalize(ubo.cameraPosition.xyz - WorldPos);
   vec3 reflect_dir = reflect(-lightDir, normal);
   float VdotR = pow(max(dot(view_dir, reflect_dir), 0.0), 32);
   vec3 specular = specular_amount * VdotR * lightColour.xyz;

   //float sss = screen_space_shadows(lightDir);
   vec4 ref = screen_space_reflections();

   float shadow = Shadow(WorldPos);
   vec3 shade = (ambient + (1.0 - shadow) * (diffuse + specular)) * color.xyz; // should be object colour
   
   float reflectivity = 0.5;
   shade = mix(shade, ref.xyz, reflectivity);

   if(debugRenderer.debugRenderTarget == 1)
   {
	   float x = texture(shadowMap, uv).x;
       outColor = vec4(vec3(x), 1.0);
   }

   else if(debugRenderer.debugRenderTarget == 2)
   {
   	  float sampleDepth = texture(depthTex, uv).x;
	  float depthVpos = LinearizeDepth(sampleDepth);
	  vec3 normal = normalize(texture(gBuffNormal, uv).xyz);
	  outColor = vec4(vec3(depthVpos), 1.0);
   }
   else 
   {
	   float sampleDepth = texture(depthTex, uv).x;
	   float depthVpos = LinearizeDepth(sampleDepth);
	   outColor = vec4(vec3(shade), 1.0);
   }
}

