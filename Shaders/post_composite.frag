#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform bool uEffectsEnabled;
uniform float uExposure;
uniform float uGamma;
uniform float uBloomStrength;
uniform float uSaturation;
uniform float uContrast;
uniform vec3 uColorTint;
uniform float uVignetteStrength;
uniform float uVignetteSoftness;
uniform float uLetterbox;
uniform vec2 uResolution;

vec3 acesFilm(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) /
                 (color * (c * color + d) + e), 0.0, 1.0);
}

void main() {
    vec4 scene = texture(uScene, vUV);
    vec3 hdr = scene.rgb + texture(uBloom, vUV).rgb * uBloomStrength;
    vec3 color = acesFilm(hdr * uExposure);
    if (uEffectsEnabled) {
        float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
        color = mix(vec3(luminance), color, uSaturation);
        color = (color - 0.5) * uContrast + 0.5;
        color *= uColorTint;

        float aspect = uResolution.x / max(uResolution.y, 1.0);
        vec2 centered = (vUV - 0.5) * vec2(aspect, 1.0);
        float edge = length(centered) / length(vec2(0.5 * aspect, 0.5));
        float vignette = 1.0 - uVignetteStrength *
            smoothstep(1.0 - uVignetteSoftness, 1.0, edge);
        color *= vignette;
    }
    color = pow(max(color, vec3(0.0)), vec3(1.0 / uGamma));
    // Cinematic letterbox bars, independent of the effects toggle.
    if (vUV.y < uLetterbox || vUV.y > 1.0 - uLetterbox) {
        color = vec3(0.0);
    }
    FragColor = vec4(color, scene.a);
}
