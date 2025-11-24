#version 330 core

in vec3 vColor;
in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    vec4 tex = texture(spriteTexture, vTexCoord);
    // If texture is missing or default white, tint by vertex color; otherwise use sampled alpha.
    FragColor = vec4(tex.rgb * vColor, tex.a);
}
