
#include <marchingCubes.hpp>

MarchingCubes::MarchingCubes(int w, int h, int d, int xr, int yr, int zr)
{
	width = w;
	height = h;
	depth = d;
	xRat = xr;
	yRat = yr;
	zRat = zr;

	num_cells = width / xRat * height / yRat * depth / zRat;

	//voxels = (char*) malloc(sizeof(char) * width / xRat * height / yRat * depth / zRat);
	voxels = (short*) malloc(sizeof(short) * num_cells);
	//load_data();
	grid = (int*) malloc(sizeof(int) * num_cells);
	for (int i = 0; i < num_cells; i++)
		grid[i] = i;

	setup();

	shaders = ShaderResources::get_instance();

	shaders->renderPass->use();
	shaders->renderPass->setInt("material.diffuse", 0);

	// Set material properties
	shaders->renderPass->setVec3("material.specular", 0.3f, 0.3f, 0.3f);
	shaders->renderPass->setFloat("material.shininess", 64.0f);

	shaders->marchingCubes->use();

	shaders->marchingCubes->setInt("WIDTH", width / xRat);
	shaders->marchingCubes->setInt("HEIGHT", height / yRat);
	shaders->marchingCubes->setInt("DEPTH", depth / zRat);

	shaders->marchingCubes->setInt("xRat", xRat);
	shaders->marchingCubes->setInt("yRat", yRat);
	shaders->marchingCubes->setInt("zRat", zRat);

	// initial geometry pass
	updateGeometry(1, false);
}

MarchingCubes::~MarchingCubes()
{
	glDeleteVertexArrays(1, &TVAO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &boxVAO);
	glDeleteTransformFeedbacks(1, &TFO);
	glDeleteBuffers(1, &TBO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &boxVBO);

	free(voxels);
	free(grid);
}

void MarchingCubes::set_voxel(int x, int y, int z, short val)
{
	if (x >= 0 && y >= 0 && z >= 0 &&
		x < width && y < height && z < depth)
		voxels[z * width * height + y * width + x] = val;
}

void MarchingCubes::load_data2()
{
	char* tmpBuf = (char*)malloc(sizeof(char) * width * height);

	for (int k = 0; k < depth; k += zRat)
	{
		//string path = "../KinectSLAM/Media/CT_Data/mrbrain-8bit0";
		std::string path = "../KinectSLAM/Media/CT_Data/1-";
		if (k + 1 < 100)
			path += "0";
		if (k + 1 < 10)
			path += "0";
		path += std::to_string(k + 1);
		path += ".dcm.tif";

		//printf("%s\n", path.c_str());

		std::ifstream myFile(path.c_str(), std::ios::in | std::ios::binary);
		myFile.read(tmpBuf, width * height);
		if (!myFile) {
			// An error occurred!
			// myFile.gcount() returns the number of bytes read.
			// calling myFile.clear() will reset the stream state
			// so it is usable again.
			printf("Unable to read CT Data File: %s, Bytes read: %d\n", path.c_str(), myFile.gcount());
		}
		for (int i = 0; i < width; i += xRat)
			for (int j = 0; j < height; j += yRat)
				voxels[k / zRat * width / xRat * height / yRat + i / xRat * height / yRat + j / yRat] =
				tmpBuf[i * height + j];
	}

	free(tmpBuf);
}

void MarchingCubes::load_data()
{
	//string path = "../KinectSLAM/Media/CT_Data/rawtest.nrrd";
	std::string path = "../KinectSLAM/Media/CT_Data/HEADW_unz.nrrd";
	//string path = "../KinectSLAM/Media/CT_Data/rabbit.nrrd";
	std::ifstream myFile(path.c_str(), std::ios::in | std::ios::binary);

	short* tmpBuf = (short*)malloc(sizeof(short) * width * height);
	if (tmpBuf == NULL) {
		printf("Not enough memory, unable to load data.\n");
		exit(-1);
	}

	for (int k = 0; k < depth; k += zRat)
	{
		myFile.seekg(sizeof(short) * k * width * height);
		myFile.read((char*)tmpBuf, sizeof(short) * width * height);

		if (!myFile)
		{
			printf("Unable to read CT Data File: %s, Bytes read: %d\n", path.c_str(), myFile.gcount());
			myFile.close();
			return;
		}

		//downsample by factor of xRat * yRat
		for (int i = 0; i < width; i += xRat)
			for (int j = 0; j < height; j += yRat)
			{
				short sum = 0;
				for (int kernx = 0; kernx < xRat; kernx++)
					for (int kerny = 0; kerny < yRat; kerny++)
						sum += tmpBuf[(i + kernx) * height + j + kerny];
				voxels[k / zRat * width / xRat * height / yRat + i / xRat * height / yRat + j / yRat] = sum / xRat / yRat;
			}
	}

	free(tmpBuf);

	myFile.close();
}

void MarchingCubes::updateGeometry(int threshold, bool reupload_voxels)
{
	shaders->marchingCubes->use();
	shaders->marchingCubes->setInt("threshold", threshold);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_BUFFER, txtr);

	// reupload voxels
	if (reupload_voxels)
	{
		glBindBuffer(GL_TEXTURE_BUFFER, buf);
		glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(short) * num_cells, voxels);
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_BUFFER, txtr2);

	glEnable(GL_RASTERIZER_DISCARD);
	//glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, query);
	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, TFO);
	glBeginTransformFeedback(GL_TRIANGLES);
	glBindVertexArray(VAO);
	glDrawArrays(GL_POINTS, 0, num_cells);
	glBindVertexArray(0);
	glEndTransformFeedback();

	//glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
	glDisable(GL_RASTERIZER_DISCARD);
	glFlush();
	//glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
	//printf("primitives count: %d\n", primitives);
}

void MarchingCubes::draw(glm::mat4 model)
{
	shaders->renderPass->use();
	shaders->renderPass->setInt("material.diffuse", 0);
	shaders->renderPass->setMat4("model", model);

	glm::mat3 normalMatrix(model);
	normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
	shaders->renderPass->setMat3("normalMatrix", normalMatrix);

	glBindVertexArray(TVAO);
	glDrawTransformFeedback(GL_TRIANGLES, TFO);
	
	// visualize bounding box of voxel field
	shaders->line->use();
	shaders->line->setVec4("lineColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	shaders->line->setMat4("model", model);

	glBindVertexArray(boxVAO);
	glDrawArrays(GL_LINES, 0, 24);

	glBindVertexArray(0);
}

void MarchingCubes::setup()
{
	// grid positions vertex array for the marching cubes geometry shader
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(int) * num_cells, grid, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 1, GL_INT/*, GL_FALSE*/, sizeof(int), (void*)0);


	// send voxel data as a texture
	glGenBuffers(1, &buf);
	glGenTextures(1, &txtr);

	glActiveTexture(GL_TEXTURE0);
	glBindBuffer(GL_TEXTURE_BUFFER, buf);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(short) * num_cells, nullptr, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, txtr);
	//glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, buf);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R16I, buf);


	// send triangles reference table for marching cubes as a texture
	// Note: this table is too big to include in the shader as a constant
	glGenBuffers(1, &buf2);
	glGenTextures(1, &txtr2);

	glActiveTexture(GL_TEXTURE1);
	glBindBuffer(GL_TEXTURE_BUFFER, buf2);
	glBufferData(GL_TEXTURE_BUFFER, 256 * 16 * sizeof(int), triTable, GL_STATIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, txtr2);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, buf2);


	// bounding box vertices
	glGenVertexArrays(1, &boxVAO);
	glGenBuffers(1, &boxVBO);

	glBindVertexArray(boxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(boundingBoxVerts), boundingBoxVerts, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);


	// transform feedback of triangle vertices from the geometry construction pass
	// used to render during the render pass 
	glGenVertexArrays(1, &TVAO);
	glGenTransformFeedbacks(1, &TFO);
	glGenBuffers(1, &TBO);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, TFO);
	glBindVertexArray(TVAO);
	glBindBuffer(GL_ARRAY_BUFFER, TBO);
	glBufferData(GL_ARRAY_BUFFER, num_cells * 15 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, TBO);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));

	glBindVertexArray(0);

	
	// query to be used to count the number of generated primitives (triangles)
	//glGenQueries(1, &query);
}

