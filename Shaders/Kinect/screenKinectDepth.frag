#version 430 core
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding = 0) uniform sampler2D depthTexture;

uniform float far_plane;

void main()
{ 
    //FragColor = texture(tex, ftexCoords);
    float r = 65.535 * texture(depthTexture, TexCoords).r / far_plane;
    //float r = texture(tex, ftexCoords).r;
    FragColor = vec4(r, r, r, 1);
}