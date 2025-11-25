#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec2 aTexCoord;

out vec4 vColor;
out vec2 vTexCoord;

uniform mat4 transform;
uniform mat4 projection;

void main() {
    vColor = aColor;
    vTexCoord = aTexCoord;
    gl_Position = projection * transform * vec4(aPos, 0.0, 1.0);
}
