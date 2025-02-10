#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 viewProjection;

out vec3 TexCoords;

void main()
{
    TexCoords = aPos;
    vec4 pos = viewProjection * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
