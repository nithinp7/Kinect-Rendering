#version 430 core

// note this is the cell index, not grid index 
layout (location = 0) in int index;

uniform int WIDTH;
uniform int HEIGHT;
uniform int DEPTH;

out ivec3 gridPos;

void main()
{
    int z = index / ((WIDTH - 1) * (HEIGHT - 1));
    int y = (index % ((WIDTH - 1) * (HEIGHT - 1))) / (WIDTH - 1);
    int x = index % (WIDTH - 1);

    gridPos = ivec3(x, y, z);
}
