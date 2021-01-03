#ifndef SHADER_RESOURCES_H
#define SHADER_RESOURCES_H

#include <shader.hpp>

// singleton pattern

class ShaderResources {

public:
	Shader* screenRGBA;
	Shader* screenGrey;
	Shader* screenDepth;
	Shader* point;
	Shader* line;
	Shader* normal;
	Shader* texture;
	Shader* skybox;
	Shader* renderPass;

	Shader* marchingCubes;
	Shader* marchingCubesVertsCount;

	Shader* kinectPointCloud;
	Shader* kinectPointCloudFiltered;
	Shader* kinectDepthTexture;
	Shader* kinectBuckets;
	Shader* kinectSavePoints;
	Shader* points2voxels;
	Shader* buckets2voxels;

	Shader* bufferSum;

	Shader* rgba2grey;
	Shader* edges;
	Shader* threshold;
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