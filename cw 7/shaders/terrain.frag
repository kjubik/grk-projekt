#version 330 core
in vec3 FragPos;  
in vec2 TexCoords; // UV coordinates
out vec4 FragColor;

uniform sampler2D terrainTexture; // Texture sampler

void main()
{
    vec4 texColor = texture(terrainTexture, TexCoords); // Sample the texture
    FragColor = texColor; // Use the sampled color
    // You can also add lighting calculations here if needed.
}
