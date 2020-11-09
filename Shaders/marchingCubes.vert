#version 430 core

struct Tri {
	vec3 pos[3];
    vec3 norm[3];
    vec4 col[3];
};

layout (location = 0) in ivec3 aPos;

out ivec3 gridPos;

uniform bool geomBypass;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gridPos = aPos;
    //gl_Position = projection * view * model * vec4(float(aPos.x), float(aPos.y), float(aPos.z), 0.0);
}
