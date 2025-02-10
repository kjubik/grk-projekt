#version 410 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;
in mat3 TBN;
in vec4 LightSpacePos; // New: Input from vertex shader

uniform sampler2D terrainTexture;
uniform sampler2D normalMap;
uniform sampler2D shadowMap; // New: Shadow map texture
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform float ambientStrength = 0.2;
uniform float specularStrength = 0.5;
uniform float shininess = 32.0;

// Function to calculate shadow
float calculateShadow(vec4 lightSpacePos) {
    vec3 projectedCoords = lightSpacePos.xyz / lightSpacePos.w; // Perspective divide
    projectedCoords = projectedCoords * 0.5 + 0.5; // Transform to [0,1] texture space

    // PCF (Percentage Closer Filtering) for smoother shadows (optional but recommended)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projectedCoords.xy + vec2(x,y) * texelSize).r;
            shadow += projectedCoords.z > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;  // Average the PCF samples

    return shadow;
}

void main()
{
    vec3 normal = texture(normalMap, TexCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);
    normal = normalize(TBN * normal);

    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, normal);

    vec3 ambient = ambientStrength * lightColor;

    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 textureColor = texture(terrainTexture, TexCoords).rgb;

    // Shadow calculation
    float shadow = calculateShadow(LightSpacePos);

    // Apply shadow to the lighting
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * textureColor; // Shadowed diffuse and specular

    FragColor = vec4(lighting, 1.0);
}