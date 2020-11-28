#version 430 core
layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D tex;

uniform float far_plane;

in vec2 ftexCoords;

void main()
{    
    //FragColor = texture(tex, ftexCoords);
    float r = 65.535 * texture(tex, ftexCoords).r / far_plane;
    //float r = texture(tex, ftexCoords).r;
    FragColor = vec4(r, r, r, 1);
}
