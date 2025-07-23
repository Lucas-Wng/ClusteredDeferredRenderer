#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

// G-buffer
uniform sampler2D gPosition;      // view-space position (vec3)
uniform sampler2D gNormal;        // view-space normal   (vec3)
uniform sampler2D gAlbedoSpec;    // albedo.rgb + spec power in .a (vec4)

// Cluster light index texture (integer 2D texture)
uniform isampler2D clusterLightTex;

// Light array (must match what C++ uploads; here max 256 lights)
struct Light {
    vec4 position_radius; // xyz = view-space pos; w = radius
    vec3 color;           // RGB intensity
    float intensity;      // scalar intensity multiplier
};
uniform Light lights[256];
uniform int numLights;

// Clustering parameters
uniform int screenWidth;
uniform int screenHeight;
uniform int CLUSTER_X;
uniform int CLUSTER_Y;
uniform int CLUSTER_Z;
uniform int MAX_LIGHTS_PER_CLUSTER;
uniform float nearPlane;
uniform float farPlane;

// Camera
uniform vec3 viewPos; // world-space, only used for spec (not shown here)

void main() {
    // 1) fetch G-buffer
    vec3 fragPosVS = texture(gPosition, TexCoords).rgb;
    vec3 N        = normalize(texture(gNormal, TexCoords).rgb);
    vec4 albSpec  = texture(gAlbedoSpec, TexCoords);
    vec3 albedo   = albSpec.rgb;
    float specPow = albSpec.a;

    // 2) compute cluster X/Y
    ivec2 pix = ivec2(gl_FragCoord.xy);
    int cx = int(float(pix.x) / float(screenWidth) * float(CLUSTER_X));
    int cy = int(float(pix.y) / float(screenHeight)* float(CLUSTER_Y));

    // 3) reconstruct view-space Z from depth buffer
    float d = gl_FragCoord.z;
    float zVS = - (farPlane * nearPlane) / (d * (farPlane - nearPlane) - farPlane);

    // 4) compute logarithmic Z slice
    float lnRatio = log(zVS/nearPlane) / log(farPlane/nearPlane);
    int cz = int(clamp(lnRatio * float(CLUSTER_Z), 0.0, float(CLUSTER_Z - 1)));

    // 5) flatten cluster index
    int clusterIdx = cx
    + cy * CLUSTER_X
    + cz * (CLUSTER_X * CLUSTER_Y);

    // 6) loop over lights assigned to this cluster
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < MAX_LIGHTS_PER_CLUSTER; ++i) {
        int li = texelFetch(clusterLightTex, ivec2(i, clusterIdx), 0).r;
        if (li < 0) break;               // no more lights
        Light L = lights[li];

        vec3 Lpos = L.position_radius.xyz;
        float radius = L.position_radius.w;
        vec3 toLight = Lpos - fragPosVS;
        float dist = length(toLight);
        vec3  Ldir = toLight / dist;

        // simple linear attenuation
        float att = max(0.0, 1.0 - dist/radius);

        // diffuse
        float diff = max(dot(N, Ldir), 0.0);

        lighting += L.color * diff * att * L.intensity;
    }

    // 7) apply to albedo
    FragColor = vec4(albedo * lighting, 1.0);
}