#version 330 core

in vec2 vUV;
in vec4 vColor;

layout(location = 0) out vec4 outColor;

uniform sampler2D uiTexture;
uniform bool alphaMask;

void main() {
    vec4 sampleColor = texture(uiTexture, vUV);
    if (alphaMask) {
        sampleColor = vec4(1.0, 1.0, 1.0, sampleColor.r);
    }
    outColor = sampleColor * vColor;
}
