#version 330 core
in vec3 FragPos;  
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D terrainTexture;

void main()
{
    vec4 texColor = texture(terrainTexture, TexCoords);
    FragColor = texColor;
}
