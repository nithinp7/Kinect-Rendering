#version 430 core
layout(location = 0) out vec4 FragColor;

flat in vec4 color;
flat in int reject;

void main()
{    
    if (reject == 1)
        discard;
    FragColor = color;
}
