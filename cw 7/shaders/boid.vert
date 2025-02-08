#version 410 core
layout (location = 0) in vec3 position;

out vec3 fragColor;

uniform vec3 inputColor;  // Add this

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragColor = inputColor;
}
