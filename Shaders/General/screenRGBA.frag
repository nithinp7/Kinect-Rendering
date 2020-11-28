#version 430 core
layout(location = 0) out vec4 FragColor;
  
layout(binding = 0) uniform sampler2D screenTexture;

in vec2 TexCoords;

void main()
{ 
    FragColor = texture(screenTexture, TexCoords);
}