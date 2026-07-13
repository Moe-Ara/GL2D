#version 330 core

in vec4 vColor;
in vec2 vTexCoord;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 OutNormal;

uniform sampler2D spriteTexture;
uniform sampler2D normalTexture;

void main() {
    vec4 tex = texture(spriteTexture, vTexCoord);
    vec3 albedo = tex.rgb * vColor.rgb;
    float alpha = tex.a * vColor.a;
    OutColor = vec4(albedo, alpha);

    vec3 sampledNormal = texture(normalTexture, vTexCoord).xyz * 2.0 - 1.0;
    vec3 n = normalize(sampledNormal);
    if (length(n) < 0.01) {
        n = vec3(0.0, 0.0, 1.0);
    }
    OutNormal = vec4(n * 0.5 + 0.5, 1.0);
}
