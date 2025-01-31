#version 430 core

layout(location = 0) in vec4 vertexPosition;
layout(location = 1) in vec4 vertexColor;

uniform mat4 transformation;

out vec4 color;

void main()
{
	color = vertexColor;
	gl_Position = transformation * vertexPosition;
}
