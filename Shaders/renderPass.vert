#version 430 core

struct Vertex {
    vec3 pos;
    vec3 norm;
    vec4 col;
};

in vec3 pos;
in vec3 norm;
in vec4 col;

//layout(location = 0) in Vertex v;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform mat3 normalMatrix;

void main()
{
    FragPos = (model * vec4(pos, 1.0)).xyz;//projection * view * model * vec4(pos, 1.0);    
    Normal = mat3(transpose(inverse(model))) * normalize(norm); //normalMatrix * normalize(norm);//mat3(transpose(inverse(model))) * normalize(norm);
    Color = col;

    gl_Position = projection * view * model * vec4(pos, 1);
}
