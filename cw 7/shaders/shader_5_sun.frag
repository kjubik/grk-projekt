#version 430 core

uniform vec3 objectColor;

in vec3 interpNormal;

void main()
{
	gl_FragColor = vec4(objectColor, 1.0);
}
