

#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform sampler2D diffuseTexture;
//uniform sampler2D specularGlossinessTexture;
//uniform sampler2D normalTexture;
//uniform sampler2D occlusionTexture;
//uniform sampler2D emissiveTexture;

void main()
{
    // ambient
    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * lightColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 lightingResult = ambient + diffuse + specular;
    vec4 texColor = texture(diffuseTexture, TexCoord);
    vec3 finalColor = lightingResult * texColor.rgb;

    FragColor = vec4(finalColor, texColor.a);
}

