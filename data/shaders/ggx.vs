#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aColor;
layout (location = 3) in vec3 aTexCoords;
layout (location = 4) in vec4 aTexProperty;
layout (location = 5) in vec2 aGloss;

flat out float TexId;
flat out vec2 TexOffset;
flat out vec2 TexSize;
out vec2 TexCoord;
out vec3 Position;
out vec3 Normal;
out vec3 Color;
out float Specular;
out float Roughness;

uniform mat4 view;
uniform mat4 projection;

void main() {
    Position = aPos;
    TexCoord = aTexCoords.xy;
    TexId = aTexCoords.z;
    TexOffset = aTexProperty.xy;
    TexSize = aTexProperty.zw;

    vec4 PosInView = view * vec4(aPos, 1.0f);

    Normal = aNormal;
    Color = aColor;

    Specular = clamp(aGloss.x, 0.0f, 1.0f);
    Roughness = clamp(aGloss.y, 0.0f, 1.0f);

    gl_Position = projection * PosInView;
}
