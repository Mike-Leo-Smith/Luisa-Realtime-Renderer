#version 410 core

layout (location = 0) out vec4 FragColor;

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

float DistributionGGX(vec3 m, vec3 n, float alpha)
{
    float cos_theta_m = dot(m, n);

    if (cos_theta_m <= 0.0f) {
        return 0.0f;
    }
    float root = alpha / (cos_theta_m * cos_theta_m * (alpha * alpha - 1.0f) + 1.0f);

    return root * root / PI;
}

float G1(vec3 v, vec3 m, vec3 n, float alpha) {

    float v_dot_n = dot(v, n);
    float v_dot_m = dot(v, m);

    if (v_dot_n * v_dot_m <= 0.0f) {
        return 0.0f;
    }

    float sqr_cos_theta_v = max(abs(v_dot_n * v_dot_n), 0.0001f);
    float sqr_tan_theta_v = (1.0f - sqr_cos_theta_v) / sqr_cos_theta_v;

    return 2.0f / (1.0f + sqrt(1.0f + alpha * alpha * sqr_tan_theta_v));
}

float Geo(vec3 i, vec3 o, vec3 m, vec3 n, float alpha) {
    return G1(i, m, n, alpha) * G1(o, m, n, alpha);
}

float fresnel(float cos_theta, float eta) {
    float sin_theta_t_sqr = eta * eta * (1.0f - cos_theta * cos_theta);

    if (sin_theta_t_sqr >= 1.0f) {
        return 1.0f;
    }

    float cos_theta_t = sqrt(1.0f - sin_theta_t_sqr);
    float rs = (eta * cos_theta - cos_theta_t) / (eta * cos_theta + cos_theta_t);
    float rp = (cos_theta - eta * cos_theta_t) / (cos_theta + eta * cos_theta_t);
    return 0.5f * (rs * rs + rp * rp);
}

void main()
{
    vec3 V = normalize(cameraPos - Position);
    vec3 N = normalize(dot(Normal, V) >= 0.0f ? Normal : -Normal);

//    FragColor = vec4(0.5f * N + 0.5f, 1.0f);
//    return;

    vec3 Albedo = Color;
    if (TexId >= 0) {
        vec2 Coord = (fract(fract(TexCoord) + 1.0f) * TexSize + TexOffset) / ${TEXTURE_MAX_SIZE};
        vec4 Sample = texture(textures, vec3(Coord, TexId));
        if (Sample.a < 0.01f) {
            discard;
        }
        Albedo = pow(Sample.rgb, vec3(2.2f));
    }

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for (int i = 0; i < LIGHT_COUNT; i++) {

        vec3 LightPosition = lights[i].Position;
        vec3 LightColor = lights[i].Color;

        // calculate per-light radiance
        vec3 L = normalize(LightPosition - Position);

        float NdotL = dot(N, L);
        float NdotV = dot(N, V);

        if (NdotL > 0.0f) {

            float distance = length(LightPosition - Position);
            float attenuation = 1.0 / (distance * distance);
            vec3 radiance = LightColor * attenuation;

            vec3 H = normalize(V + L);

            // Cook-Torrance BRDF
            float D = DistributionGGX(H, N, Roughness);
            float G   = Geo(V, L, H, N, Roughness);

            float F = fresnel(dot(H, V), 1.5f);

            float nominator = D * G * F;
            float denominator = 4 * abs(NdotV);
            float specular = Specular * nominator / max(denominator, 0.001);
            vec3 diffuse = (1.0f - Specular) * Albedo * abs(NdotL) / PI;

            Lo += (diffuse + specular) * radiance;
        }
    }
    FragColor = vec4(Lo, 1.0f);
}
