#version 430 core

layout (location = 0) in int index;

uniform int WIDTH;
uniform int HEIGHT;
uniform int DEPTH;

out ivec3 gridPos;

void main()
{
    int z = index / (WIDTH * HEIGHT);
    int y = (index % (WIDTH * HEIGHT)) / WIDTH;
    int x = index % WIDTH;

    gridPos = ivec3(x, y, z);
}
