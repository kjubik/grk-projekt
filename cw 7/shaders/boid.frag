#version 410 core

in vec3 FragPos;
in vec3 Normal;
in vec2 FragTexCoords;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform sampler2D colorTexture;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambient = 0.2 * lightColor;
    vec3 diffuse = diff * lightColor;
    vec3 specular = spec * lightColor;

    vec3 textureColor = texture(colorTexture, vec2(FragTexCoords.x, 1.0 - FragTexCoords.y)).rgb;

    vec3 result = (ambient + diffuse + specular) * textureColor;

    FragColor = vec4(result, 1.0);
}
