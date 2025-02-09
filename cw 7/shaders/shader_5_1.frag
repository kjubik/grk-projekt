#version 430 core

float AMBIENT = 0.1;

uniform vec3 color;
uniform vec3 lightPos;
uniform sampler2D shadowMap; 
uniform mat4 lightSpaceMatrix;

in vec3 vecNormal;
in vec3 worldPos;

out vec4 outColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float shadow = currentDepth > closestDepth + 0.005 ? 1.0 : 0.0; 
    return shadow;
}

void main()
{
	vec3 lightDir = normalize(lightPos-worldPos);
	vec3 normal = normalize(vecNormal);
	float diffuse=max(0,dot(normal,lightDir));

    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(worldPos, 1.0);

    float shadow = ShadowCalculation(fragPosLightSpace);

	outColor = vec4(color*min(1,AMBIENT+diffuse), 1.0);
}
