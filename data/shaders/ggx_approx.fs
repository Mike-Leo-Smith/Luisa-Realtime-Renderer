#version 410 core

layout (location = 0) out highp vec4 FragColor;

flat in float TexId;
flat in vec2 TexOffset;
flat in vec2 TexSize;
in vec2 TexCoord;
in vec3 Position;
in vec3 Normal;
in vec3 Color;
in float Specular;
in float Roughness;

uniform sampler2DArray textures;
uniform vec3 cameraPos;

const int LIGHT_COUNT = ${LIGHT_COUNT};
const float PI = 3.1415926536f;

struct Light {
    vec3 Position;
    vec3 Color;
};

uniform Light lights[LIGHT_COUNT];

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec3 V = normalize(cameraPos - Position);
    vec3 N = normalize(dot(Normal, V) >= 0.0f ? Normal : -Normal);
    float Metallic = 0.0f;
    float Roughness = sqrt(Roughness);

    vec3 Albedo = Color;
    if (TexId >= 0.0f) {
        vec2 Coord = (fract(fract(TexCoord) + 1.0f) * TexSize + TexOffset) / float(${TEXTURE_MAX_SIZE});
        vec4 Sample = texture(textures, vec3(Coord, TexId));
        if (Sample.a < 0.01f) {
            discard;
        }
        Albedo = pow(Sample.rgb, vec3(2.2f));
    }

    vec3 F0 = mix(vec3(0.04), Albedo, Metallic);
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < LIGHT_COUNT; i++) {

        vec3 LightPosition = lights[i].Position;
        vec3 LightColor = lights[i].Color;

        // calculate per-light radiance
        vec3 L = normalize(LightPosition - Position);

        float distance = length(LightPosition - Position);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = LightColor * attenuation;

        vec3 H = normalize(V + L);

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, Roughness);
        float G = GeometrySmith(N, V, L, Roughness);
        vec3 F = fresnelSchlickRoughness(max(dot(H, V), 0.0), F0, Roughness);

        vec3 Ks = vec3(Specular);
        vec3 Kd = vec3(1.0) - Ks;
        Kd *= 1.0 - Metallic;

        vec3 nominator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) + 0.001;
        vec3 specular     = nominator / denominator;

        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (Kd * Albedo / PI * NdotL + specular) * radiance;
    }

    FragColor = vec4(Lo, 1.0f);

}
