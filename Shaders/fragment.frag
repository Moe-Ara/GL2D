#version 330 core

in vec2 vTexCoord;         // Correct input variable
out vec4 FragColor;

uniform sampler2D spriteTexture;

void main() {
    FragColor = texture(spriteTexture, vTexCoord);
}
