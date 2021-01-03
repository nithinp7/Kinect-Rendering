#ifndef PC_REG_H
#define PC_REG_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <limits>

#include <shader.hpp>
#include <shaderResources.hpp>

class PointCloudRegistration {

public:

	PointCloudRegistration(int pointCloudSize);
	~PointCloudRegistration();
	void setModel(glm::vec4* model, glm::mat4 modelMat);
	void registerToModel(glm::vec4* pointCloud);
	void clear() {
		_modelSet = false;
		_pointCloudSet = false;
		_currentCloud = 0;
	}
	void draw();
	bool getModelSet() { return _modelSet; }
	bool getPointCloudSet() { return _pointCloudSet; }

private:

	// global shader manager
	ShaderResources* _shaders;

	bool debugModel = true;
	bool debugPC = true;

	int _pointCloudSize;
	const int ICP_ITERS = 10;

	// uniform grid for model point cloud
	glm::mat4 _modelMat;
	glm::mat4 _modelInvMat;
	int* _bucketHeads;
	int* _bucketNodes;
	const int _gridWidth = 20;
	const int _gridSize = _gridWidth * _gridWidth * _gridWidth;

	// model point cloud
	bool _modelSet = false;
	glm::vec4* _model;
	unsigned int _modelVAO;
	unsigned int _modelVBO;

	// the point cloud to register 
	static const int CLOUDS_NUM = 5;
	int _currentCloud = 0;
	bool _pointCloudSet = false;
	glm::vec4* _pointCloud[CLOUDS_NUM];
	unsigned int _pointCloudVAO[CLOUDS_NUM];
	unsigned int _pointCloudVBO[CLOUDS_NUM];

	glm::mat4 _registeredTransform[CLOUDS_NUM];

	glm::vec4 worldToGrid(glm::vec4 world) {
		return  (_modelInvMat * world + glm::vec4(0.5f, 0.5f, 0.5f, 0.0f)) * glm::vec4(_gridWidth, _gridWidth, _gridWidth, 1.0f);
	}

	glm::vec4 worldToGrid(glm::vec3 world) {
		return  worldToGrid(glm::vec4(world, 1.0f));
	}

	struct ClosestPoint {
		int index;
		float dist;

		ClosestPoint() {
			index = -1;
			dist = std::numeric_limits<float>::max();
		}
	};

	void getClosestPoint(glm::vec3 pos, glm::ivec3 cell, ClosestPoint* cp) {
		if (cell.x < 0 || cell.y < 0 || cell.z < 0 ||
			cell.x >= _gridWidth || cell.y >= _gridWidth || cell.z >= _gridWidth)
			return;
		int cellIndex = (cell.z * _gridWidth + cell.y) * _gridWidth + cell.x;
		int next = _bucketHeads[cellIndex];
		while (next != -1) {
			float dist = glm::length(glm::vec3(_model[2 * next]) - pos);
			if (dist < cp->dist) {
				cp->index = next;
				cp->dist = dist;
			}
			next = _bucketNodes[next];
		}
	}
};

#endif