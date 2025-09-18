#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D  gPosition;      // view-space position
uniform sampler2D  gNormal;        // view-space normal
uniform sampler2D  gAlbedoSpec;    // albedo.rgb (sRGB?) + gloss in .a
uniform isampler2D clusterLightTex;

struct Light {
    vec4 position_radius; // xyz world-space, w radius
    vec3 color;           // 0..1 radiance scale
    float intensity;
};
uniform Light lights[256];
uniform int   numLights;

uniform int screenWidth, screenHeight;
uniform int CLUSTER_X, CLUSTER_Y, CLUSTER_Z, MAX_LIGHTS_PER_CLUSTER;
uniform float nearPlane, farPlane;
uniform mat4 view;

void main() {
    // G-buffer fetch
    vec3 fragPosVS = texture(gPosition, TexCoords).rgb;
    vec3 N = normalize(texture(gNormal, TexCoords).rgb);

    vec4 albSpec = texture(gAlbedoSpec, TexCoords);
    // Albedo is already in linear space (loaded as sRGB)
    vec3 albedo = albSpec.rgb;
    float gloss01 = clamp(albSpec.a, 0.0, 1.0);
    float shininess = mix(8.0, 128.0, gloss01);
    

    // Cluster coords (use G-buffer Z, not gl_FragCoord.z)
    ivec2 pix = ivec2(gl_FragCoord.xy);
    int cx = clamp(int(float(pix.x) / float(screenWidth)  * float(CLUSTER_X)), 0, CLUSTER_X - 1);
    int cy = clamp(int(float(pix.y) / float(screenHeight) * float(CLUSTER_Y)), 0, CLUSTER_Y - 1);

    // fragPosVS.z is negative in view space (pointing towards -Z)
    float zVSpos = max(1e-6, -fragPosVS.z); // positive view distance
    float lnRatio = log(zVSpos / nearPlane) / log(farPlane / nearPlane);
    int cz = int(clamp(lnRatio * float(CLUSTER_Z), 0.0, float(CLUSTER_Z - 1)));

    int clusterIdx = cx + cy * CLUSTER_X + cz * (CLUSTER_X * CLUSTER_Y);

    vec3 V = normalize(-fragPosVS);
    vec3 lighting = albedo * 0.1; // Add ambient lighting

    for (int i = 0; i < MAX_LIGHTS_PER_CLUSTER; ++i) {
        int li = texelFetch(clusterLightTex, ivec2(i, clusterIdx), 0).r;
        if (li < 0 || li >= numLights) break;

        Light L = lights[li];
        vec3 LposWS = L.position_radius.xyz;
        float radius = max(L.position_radius.w, 1e-3);
        
        // Transform light position to view space
        vec3 Lpos = (view * vec4(LposWS, 1.0)).xyz;

        vec3 toLight = Lpos - fragPosVS;
        float dist = max(length(toLight), 1e-4);
        vec3  Ldir = toLight / dist;

        // Improved attenuation with better falloff
        float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
        // Apply radius-based cutoff
        att *= smoothstep(radius, radius * 0.8, dist);

        float diff = max(dot(N, Ldir), 0.0);
        vec3  H    = normalize(Ldir + V);
        float spec = pow(max(dot(N, H), 0.0), shininess);

        vec3 diffuse  = albedo * diff;
        vec3 specular = vec3(spec); // Pure specular highlight

        lighting += L.color * L.intensity * att * (diffuse + specular);
    }

    // Optional: encode back to sRGB if default framebuffer is sRGB-disabled
    // FragColor = vec4(pow(lighting, vec3(1.0/2.2)), 1.0);
    FragColor = vec4(lighting, 1.0);
}