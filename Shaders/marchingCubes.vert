#version 430 core

layout (location = 0) in ivec3 aPos;

out ivec3 gridPos;

void main()
{
    gridPos = aPos;
}
