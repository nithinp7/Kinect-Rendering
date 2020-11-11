#version 430 core
layout(location = 0) out vec4 FragColor;

in vec4 vcolor;

void main()
{    
    FragColor = vcolor;
}
