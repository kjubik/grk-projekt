#version 430 core

uniform vec3 objectColor;
in vec3 normal;

out vec4 out_color;

void main()
{
	out_color = vec4(normal, 1.	);
}
