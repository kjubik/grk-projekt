#version 430 core

layout(location = 0) in vec4 vertexPosition;

uniform mat4 transformation;

void main()
{
	gl_Position = transformation * vertexPosition;
}
