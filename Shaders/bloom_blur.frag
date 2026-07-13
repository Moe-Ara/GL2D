#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uImage;
uniform vec2 uDirection;
uniform vec2 uTexelSize;

void main() {
    vec2 offset = uDirection * uTexelSize;
    vec3 result = texture(uImage, vUV).rgb * 0.227027;
    result += texture(uImage, vUV + offset * 1.384615).rgb * 0.316216;
    result += texture(uImage, vUV - offset * 1.384615).rgb * 0.316216;
    result += texture(uImage, vUV + offset * 3.230769).rgb * 0.070270;
    result += texture(uImage, vUV - offset * 3.230769).rgb * 0.070270;
    FragColor = vec4(result, 1.0);
}
