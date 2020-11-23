#version 430 core
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D tex;

in vec2 ftexCoords;

void main()
{    
    FragColor = texture(tex, ftexCoords);
}
