#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aTangent;

out VS_OUT {
    vec3 FragPos;   // world-space position
    vec2 TexCoord;
    mat3 TBN;       // tangent-bitangent-normal matrix
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    // World position
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;

    // Normal & tangent → world space
    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);
    vec3 T = normalize(mat3(model) * aTangent.xyz);

    // Gram-Schmidt orthogonalization to avoid skewed T
    T = normalize(T - dot(T, N) * N);

    // Bitangent reconstructed with handedness
    vec3 B = cross(N, T) * aTangent.w;

    vs_out.TBN = mat3(T, B, N);

    vs_out.TexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}