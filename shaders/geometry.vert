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
uniform bool hasTangents;

void main()
{
    // World position
    vec4 worldPos = model * vec4(aPos, 1.0);
    vs_out.FragPos = worldPos.xyz;

    vec3 N = normalize(mat3(transpose(inverse(model))) * aNormal);

    vec3 T, B;
    if (hasTangents) {
        T = normalize(mat3(model) * aTangent.xyz);
        T = normalize(T - dot(T, N) * N);
        B = cross(N, T) * aTangent.w;
    } else {
        // Build an arbitrary orthonormal frame around N so TBN[2] is always valid
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
        B = cross(N, T);
    }

    vs_out.TBN = mat3(T, B, N);

    vs_out.TexCoord = aTexCoord;

    gl_Position = projection * view * worldPos;
}