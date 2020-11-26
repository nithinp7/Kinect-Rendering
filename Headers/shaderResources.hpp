#ifndef SHADER_RESOURCES_H
#define SHADER_RESOURCES_H

#include <shader.hpp>

// singleton pattern

class ShaderResources {

public:
	Shader* screen;
	Shader* screenDepth;
	Shader* point;
	Shader* line;
	Shader* normal;
	Shader* texture;
	Shader* marchingCubes;
	Shader* renderPass;
	Shader* skybox;
	Shader* kinectPointCloud;
	Shader* kinectDepthTexture;
	Shader* kinectVoxelize;
	Shader* points2voxels;
	Shader* bufferSum;
	Shader* test;

	static ShaderResources* get_instance();
	static void reset_instance();
	void update(glm::mat4 view, glm::mat4 projection);

private:
	static ShaderResources* inst;

	ShaderResources();
	~ShaderResources();
};

#endif