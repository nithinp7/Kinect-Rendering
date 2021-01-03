#include <pointCloudRegistration.hpp>

PointCloudRegistration::PointCloudRegistration(int pointCloudSize) {
	_pointCloudSize = pointCloudSize;

	_model = (glm::vec4*)malloc(2 * sizeof(glm::vec4) * _pointCloudSize);

	_bucketHeads = (int*)malloc(sizeof(int) * _gridSize);
	_bucketNodes = (int*)malloc(sizeof(int) * _pointCloudSize);

	glGenVertexArrays(1, &_modelVAO);
	glGenBuffers(1, &_modelVBO);

	glBindVertexArray(_modelVAO);
	glBindBuffer(GL_ARRAY_BUFFER, _modelVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec4) * _pointCloudSize, nullptr, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), (void*)sizeof(glm::vec4));

	glGenVertexArrays(CLOUDS_NUM, _pointCloudVAO);
	glGenBuffers(CLOUDS_NUM, _pointCloudVBO);

	for (int i = 0; i < CLOUDS_NUM; ++i) {
		_pointCloud[i] = (glm::vec4*)malloc(2 * sizeof(glm::vec4) * _pointCloudSize);

		glBindVertexArray(_pointCloudVAO[i]);
		glBindBuffer(GL_ARRAY_BUFFER, _pointCloudVBO[i]);
		glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec4) * _pointCloudSize, nullptr, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), (void*)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 2 * sizeof(glm::vec4), (void*)sizeof(glm::vec4));
	}

	_shaders = ShaderResources::get_instance();
}

PointCloudRegistration::~PointCloudRegistration() {
	free(_model);
	free(_bucketHeads);
	free(_bucketNodes);
	free(_pointCloud);
	glDeleteVertexArrays(1, &_modelVAO);
	glDeleteVertexArrays(CLOUDS_NUM, _pointCloudVAO);
	glDeleteBuffers(1, &_modelVBO);
	glDeleteBuffers(CLOUDS_NUM, _pointCloudVBO);
}

void PointCloudRegistration::setModel(glm::vec4* model, glm::mat4 modelMat) {
	_modelSet = true;
	_modelMat = modelMat;
	_modelInvMat = glm::inverse(_modelMat);
	memcpy(_model, model, 2 * sizeof(glm::vec4) * _pointCloudSize);
	
	memset(_bucketHeads, 0xff, sizeof(int) * _gridSize);
	memset(_bucketNodes, 0xff, sizeof(int) * _pointCloudSize);
	for (int i = 0; i < _pointCloudSize; ++i) {
		glm::ivec3 grid = worldToGrid(_model[2 * i]);

		if (grid.x < 0 || grid.y < 0 || grid.z < 0 ||
			grid.x >= _gridWidth || grid.y >= _gridWidth || grid.z >= _gridWidth)
			continue;

		// TODO: remove debug visualization
		if (debugModel)
			_model[2 * i + 1].r = 1.0f;//glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);

		int cellIndex = (grid.z * _gridWidth + grid.y) * _gridWidth + grid.x;

		// put the current head of the cell list as the next for this point node 
		_bucketNodes[i] = _bucketHeads[cellIndex];
		// put this point node as the new head of the cell list
		_bucketHeads[cellIndex] = i;
	}

	glBindBuffer(GL_ARRAY_BUFFER, _modelVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec4) * _pointCloudSize, _model);
}

void PointCloudRegistration::registerToModel(glm::vec4* pointCloud) {
	if (_currentCloud == CLOUDS_NUM)
		return;

	_pointCloudSet = true;
	memcpy(_pointCloud[_currentCloud], pointCloud, 2 * sizeof(glm::vec4) * _pointCloudSize);

	glm::vec4 com;

	for (int i = 0; i < _pointCloudSize; ++i) {
		com += _pointCloud[_currentCloud][2 * i];
		// TODO: remove debug visualization
		if (debugPC)
			_pointCloud[_currentCloud][2 * i + 1].b = 1.0f;// = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	}

	com /= _pointCloudSize;
	//com = worldToGrid(com);
	com.w = 1.0f;

	glm::mat4 T = glm::mat4();

	int dx[] = { 0, 0 };
	int dy[] = { 0, 0 };
	int dz[] = { 0, 0 };
	for (int iter = 0; iter < ICP_ITERS; ++iter) {
		for (int i = 0; i < _pointCloudSize; i += 20) {
			glm::vec3 rt_com = T * com;
			glm::vec3 pos = T * _pointCloud[_currentCloud][2 * i];
			glm::vec3 gridf = worldToGrid(pos);
			glm::ivec3 gridi = gridf;
			glm::vec3 octant = gridf - glm::vec3(gridi);

			if (gridi.x < 0 || gridi.y < 0 || gridi.z < 0 ||
				gridi.x >= _gridWidth || gridi.y >= _gridWidth || gridi.z >= _gridWidth)
				continue;

			dx[1] = 2 * (octant.x > 0.5) - 1;
			dy[1] = 2 * (octant.y > 0.5) - 1;
			dz[1] = 2 * (octant.z > 0.5) - 1;
			ClosestPoint cp;
			for (int a = 0; a < 2; ++a)
				for (int b = 0; b < 2; ++b)
					for (int c = 0; c < 2; ++c)
						getClosestPoint(pos, gridi + glm::ivec3(dx[a], dy[b], dz[c]), &cp);

			if (cp.index == -1){// || cp.dist > 5.0f) {
				if (debugPC)
					_pointCloud[_currentCloud][2 * i + 1].g = 1.0f;// = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
				continue;
			}

			glm::vec3 closest = _model[2 * cp.index];
			// experiment with different attractive fields (coulomb based ?)
			glm::vec3 dif = glm::normalize(closest - pos);
			
			// need to project impulse onto and perpendicular to the center of mass
			glm::vec3 r = pos - rt_com;
			//glm::vec3 dpos = r * glm::dot(dif, r) / glm::dot(r, r);
			glm::vec3 dtheta = -glm::cross(dif, r);

			T = glm::translate(T, rt_com);
			T = glm::rotate(T, 0.0001f * length(dtheta), dtheta);
			T = glm::translate(T, -rt_com);
			T = glm::translate(T, 0.001f * dif);
		}
	}

	_registeredTransform[_currentCloud] = T;

	glBindBuffer(GL_ARRAY_BUFFER, _pointCloudVBO[_currentCloud]);
	glBufferSubData(GL_ARRAY_BUFFER, 0, 2 * sizeof(glm::vec4) * _pointCloudSize, _pointCloud[_currentCloud]);

	++_currentCloud;
}

void PointCloudRegistration::draw() {
	_shaders->point->use();
	if (_modelSet) {
		_shaders->point->setMat4("model", glm::mat4());
		glBindVertexArray(_modelVAO);
		glDrawArrays(GL_POINTS, 0, _pointCloudSize);
	}
	if (_pointCloudSet) {
		for (int i = 0; i < _currentCloud; i++) {
			_shaders->point->setMat4("model", _registeredTransform[i]);
			glBindVertexArray(_pointCloudVAO[i]);
			glDrawArrays(GL_POINTS, 0, _pointCloudSize);
		}
	}
	glBindVertexArray(0);
}