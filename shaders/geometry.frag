#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in VS_OUT {
    vec3 FragPos;   // world-space position
    vec2 TexCoord;
    mat3 TBN;       // tangent-bitangent-normal
} fs_in;

uniform mat4 view;

uniform sampler2D diffuseTexture;
uniform sampler2D specularGlossinessTexture;
uniform sampler2D normalTexture;
uniform sampler2D occlusionTexture; // unused in G-buffer (optional)
uniform sampler2D emissiveTexture;  // unused in G-buffer (optional)

uniform float specularStrength = 0.5;

void main()
{
    // Position in view space
    vec3 viewPos = vec3(view * vec4(fs_in.FragPos, 1.0));
    gPosition = viewPos;

    // Normal mapping (tangent → view space)
    vec3 texNormal = texture(normalTexture, fs_in.TexCoord).rgb;
    texNormal = normalize(texNormal * 2.0 - 1.0);   // [0,1] → [-1,1]
    vec3 worldNormal = normalize(fs_in.TBN * texNormal);
    vec3 viewNormal  = normalize(mat3(view) * worldNormal);
    gNormal = viewNormal;

    // Albedo
    vec3 albedo = texture(diffuseTexture, fs_in.TexCoord).rgb;

    // Specular strength from gloss map (fallback to uniform)
    float specGloss = texture(specularGlossinessTexture, fs_in.TexCoord).r;
    float specular = mix(specularStrength, specGloss, step(0.01, specGloss));

    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a   = specular;
}