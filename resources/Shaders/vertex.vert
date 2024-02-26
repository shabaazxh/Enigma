#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

layout( set = 0, binding = 0 ) uniform UScene 
{ 
	mat4 camera; 
	mat4 projection; 
	mat4 projCam; 
} uScene; 


layout(location = 0) out vec3 fragColor;

void main()
{
	gl_Position = uScene.projCam * vec4(position, 1.0f);
	fragColor = color;
}