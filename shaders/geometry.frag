


#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;

uniform sampler2D diffuseTexture;
uniform float specularStrength = 0.5;
uniform sampler2D specularGlossinessTexture;
uniform sampler2D normalTexture;
uniform sampler2D occlusionTexture;
uniform sampler2D emissiveTexture;


void main() {
    // Sample from diffuse texture
    vec3 albedo = texture(diffuseTexture, TexCoord).rgb;

    // Sample specular glossiness map's red channel as a rough approximation (fallback to specularStrength if unavailable)
    float specGloss = texture(specularGlossinessTexture, TexCoord).r;
    float specular = mix(specularStrength, specGloss, step(0.01, specGloss)); // use specGloss if it's non-zero

    // Sample and transform normal if normal map is available
    vec3 norm = normalize(Normal);
    vec3 mappedNormal = texture(normalTexture, TexCoord).rgb;
    if (length(mappedNormal) > 0.01) {
        mappedNormal = normalize(mappedNormal * 2.0 - 1.0); // map from [0,1] to [-1,1]
        norm = normalize(mappedNormal);
    }

    gPosition = FragPos;
    gNormal = norm;
    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = specular;
}
