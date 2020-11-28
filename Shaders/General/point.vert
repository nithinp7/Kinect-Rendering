#version 430 core

layout(location = 0) in vec3 pos;
layout(location = 1) in vec4 col;

out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    //FragPos = pos;//(model * vec4(pos, 1.0)).xyz;
    gl_Position = projection * view * model * vec4(pos, 1);
}
