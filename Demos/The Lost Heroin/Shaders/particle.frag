#version 330 core

in vec4 vColor;
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    vec4 tex = texture(spriteTexture, vTexCoord);
    // Radial falloff to avoid hard square particles when using the default white texture.
    // Normalize radius so the quad corner maps to 1.0.
    float r = length(vTexCoord - vec2(0.5));
    float rn = clamp(r * 1.41421356, 0.0, 1.0);
    float soft = 1.0 - smoothstep(0.4, 1.0, rn);
    float alpha = tex.a * vColor.a * soft;
    FragColor = vec4(tex.rgb * vColor.rgb, alpha);
}
