#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 texCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 ftexCoords;

void main()
{
    ftexCoords = vec2(texCoords.x, 1-texCoords.y);
    gl_Position = projection * view * model * vec4(pos, 1);
}
