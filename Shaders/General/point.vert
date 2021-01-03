#version 430 core

layout(location = 0) in vec4 pos;
layout(location = 1) in vec4 col;

out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    Color = col;
    gl_Position = projection * view * model * pos;
}
