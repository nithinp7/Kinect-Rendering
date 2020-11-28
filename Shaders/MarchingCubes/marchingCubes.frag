#version 430 core
layout(location = 0) out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    vec3 specular;
    float shininess;
}; 

in vec3 FragPos;
in vec3 Normal;
in vec4 Color;

void main()
{    
    FragColor = vec4(1, 0, 0, 1);
}

