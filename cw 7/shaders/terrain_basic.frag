#version 410 core

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
in vec4 FragPosLightSpace;

uniform sampler2D terrainTexture;
uniform sampler2D normalMap;
uniform sampler2D shadowMap;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

out vec4 FragColor;

void main()
{
    vec4 texColor = texture(terrainTexture, TexCoords);
    FragColor = texColor;
}
