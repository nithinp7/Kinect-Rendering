#version 430

const ivec3 cornerTable [] = {
		ivec3(0, 0, 0),
		ivec3(1, 0, 0),
		ivec3(1, 1, 0),
		ivec3(0, 1, 0),
		ivec3(0, 0, 1),
		ivec3(1, 0, 1),
		ivec3(1, 1, 1),
		ivec3(0, 1, 1)
};
const vec3 edgeTable[][2] = {
		{ vec3(0.0, 0.0, 0.0), vec3(1.0, 0.0, 0.0) },
		{ vec3(1.0, 0.0, 0.0), vec3(1.0, 1.0, 0.0) },
		{ vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0) },
		{ vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0) },
		{ vec3(0.0, 0.0, 1.0), vec3(1.0, 0.0, 1.0) },
		{ vec3(1.0, 0.0, 1.0), vec3(1.0, 1.0, 1.0) },
		{ vec3(0.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0) },
		{ vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 1.0) },
		{ vec3(0.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0) },
		{ vec3(1.0, 0.0, 0.0), vec3(1.0, 0.0, 1.0) },
		{ vec3(1.0, 1.0, 0.0), vec3(1.0, 1.0, 1.0) },
		{ vec3(0.0, 1.0, 0.0), vec3(0.0, 1.0, 1.0) }
};
const int edgeToCornerTable[][2] = {
	{ 0, 1 },
	{ 1, 2 },
	{ 3, 2 },
	{ 0, 3 },
	{ 4, 5 },
	{ 5, 6 },
	{ 7, 6 },
	{ 4, 7 },
	{ 0, 4 },
	{ 1, 5 },
	{ 2, 6 },
	{ 3, 7 }
};

layout(binding = 0) uniform isamplerBuffer voxels;
layout(binding = 1) uniform isamplerBuffer voxels_col;
layout(binding = 2) uniform isamplerBuffer triTableTex;

uniform int WIDTH;
uniform int HEIGHT;
uniform int DEPTH;

uniform int threshold;

in ivec3 gridPos[];

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;

int cube[8];
unsigned int cols[8];

int get_voxel(int x, int y, int z)
{
	return int(texelFetch(voxels, z * WIDTH * HEIGHT + y * WIDTH + x).r);
}

unsigned int get_color(int x, int y, int z)
{
	return unsigned int(texelFetch(voxels_col, z * WIDTH * HEIGHT + y * WIDTH + x).r);
}

// TODO: might not need safe access, texelFetch might be able to wrap automatically 
float get_voxel_safe(int x, int y, int z) {

	if (x < 0 || y < 0 || z < 0 ||
		x >= WIDTH || y >= HEIGHT || z >= DEPTH)
		return 0.0;
	
	return float(get_voxel(x, y, z));
}

void get_cube(ivec3 upos)
{
	for (int i = 0; i < 8; i++)
	{
		ivec3 v = upos + cornerTable[i];
		cube[i] = get_voxel(v.x, v.y, v.z);
		cols[i] = get_color(v.x, v.y, v.z);
	}
}

int triTable(int i, int j)
{
	return int(texelFetch(triTableTex, i*16 + j).r);
}

vec3 get_gradient(int x, int y, int z) {
	return normalize(vec3(
		get_voxel_safe(x + 1, y, z) - get_voxel_safe(x - 1, y, z),
		get_voxel_safe(x, y + 1, z) - get_voxel_safe(x, y - 1, z),
		get_voxel_safe(x, y, z + 1) - get_voxel_safe(x, y, z - 1)
	));
}

layout(points) in;
layout(triangle_strip, max_vertices = 15) out;
void main()
{
	vec3 pos = vec3(float(gridPos[0].x), float(gridPos[0].y), float(gridPos[0].z));
	
	vec3 offs = vec3(0.0, 0.0, 0.0);

	get_cube(gridPos[0]);
	
	int config = 0;
	for (int i = 0; i < 8; i++) 
		if (cube[i] > threshold)
			config |= 1 << i;

	if (config == 0 || config == 255)
		return;
	
	int edge = 0;
	for (int i = 0; i < 5; i++)
	{
		for (int p = 0; p < 3; p++)
		{
			int index = triTable(config, edge);
						
			if (index == -1)
				return;

			int i0 = edgeToCornerTable[index][0];
			int i1 = edgeToCornerTable[index][1];

			ivec3 c0 = cornerTable[i0];
			ivec3 c1 = cornerTable[i1];

			float d0 = cube[i0] - threshold;
			float d1 = cube[i1] - threshold;

			unsigned int col0_int = cols[i0];
			unsigned int col1_int = cols[i1];

			vec4 col0 = vec4(col0_int >> 24, col0_int >> 16 & 0xff, col0_int >> 8 & 0xff, col0_int & 0xff);
			vec4 col1 = vec4(col1_int >> 24, col1_int >> 16 & 0xff, col1_int >> 8 & 0xff, col1_int & 0xff);

			vec3 v0 = edgeTable[index][0];
			vec3 v1 = edgeTable[index][1];
						
			vec3 vertPos = offs + pos + -d0 * (v1 - v0) / (d1 - d0) + v0;
			Color = -d0 * (col1 - col0) / (d1 - d0) + col0;
			Color = vec4(Color.rgb / 255.0, 1);
			
			vec3 g0 = get_gradient(gridPos[0].x + c0.x, gridPos[0].y + c0.y, gridPos[0].z + c0.z);
			vec3 g1 = get_gradient(gridPos[0].x + c1.x, gridPos[0].y + c1.y, gridPos[0].z + c1.z);

			vec3 g = -d0 * (g1 - g0) / (d1 - d0) + g0;

			//FragPos = /*0.1 *  */vertPos;// * vec3(xRat, -yRat, 3 * zRat);
			FragPos = vertPos * vec3(1.0 / WIDTH, 1.0 / HEIGHT, 1.0 / DEPTH) - vec3(0.5, 0.5, 0.5);
			Normal = g;
			//Color = vec4(0.8, 0.4, 0.6, 1.0);

			EmitVertex();
						
			edge++;
		}

		EndPrimitive();
	}
}