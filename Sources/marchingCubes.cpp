
#include <marchingCubes.hpp>

MarchingCubes::MarchingCubes(int w, int h, int d)
{
	width = w;
	height = h;
	depth = d;

	grid_size = width * height * depth;
	num_cells = (width - 1) * (height - 1) * (depth - 1);

	//voxels = (char*) malloc(sizeof(char) * num_cells);
	voxels = (short*) malloc(sizeof(short) * grid_size);
	voxel_colors = (unsigned int*) malloc(sizeof(unsigned int) * grid_size);
	cells_verts = (int*) malloc(sizeof(int) * num_cells);

	for (int i = 0; i < num_cells; i++) 
		cells_verts[i] = i;

	setup();

	shaders = ShaderResources::get_instance();

	shaders->renderPass->use();

	// Set material properties
	shaders->renderPass->setInt("material.diffuse", 0);
	shaders->renderPass->setVec3("material.specular", 0.3f, 0.3f, 0.3f);
	shaders->renderPass->setFloat("material.shininess", 64.0f);

	shaders->marchingCubes->use();

	shaders->marchingCubes->setInt("WIDTH", width);
	shaders->marchingCubes->setInt("HEIGHT", height);
	shaders->marchingCubes->setInt("DEPTH", depth);


	shaders->marchingCubesVertsCount->use();

	shaders->marchingCubesVertsCount->setInt("WIDTH", width);
	shaders->marchingCubesVertsCount->setInt("HEIGHT", height);
	shaders->marchingCubesVertsCount->setInt("DEPTH", depth);


	// initial geometry pass
	updateGeometry(25, true);
}

MarchingCubes::~MarchingCubes()
{
	glDeleteVertexArrays(1, &TVAO);
	glDeleteVertexArrays(1, &VAO);
	glDeleteVertexArrays(1, &boxVAO);

	glDeleteTransformFeedbacks(1, &TFO);

	glDeleteTextures(1, &voxels_txt);
	glDeleteTextures(1, &voxel_colors_txt);
	glDeleteTextures(1, &tri_table_txt);
	glDeleteTextures(1, &verts_count_txt);

	glDeleteBuffers(1, &TBO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &boxVBO);
	glDeleteBuffers(1, &voxels_buf);
	glDeleteBuffers(1, &voxel_colors_buf);
	glDeleteBuffers(1, &tri_table_buf);
	glDeleteBuffers(1, &verts_count_buf);

	free(voxels);
	free(voxel_colors);
	free(cells_verts);
}

void MarchingCubes::set_voxel(int x, int y, int z, short val)
{
	if (x >= 0 && y >= 0 && z >= 0 &&
		x < width && y < height && z < depth)
		voxels[z * width * height + y * width + x] = val;
}

void MarchingCubes::updateGeometry(int threshold, bool reupload_voxels)
{
	if (reupload_voxels)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_BUFFER, voxels_txt);
		glBindBuffer(GL_TEXTURE_BUFFER, voxels_buf);
		glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(short) * grid_size, voxels);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_BUFFER, voxel_colors_txt);
		glBindBuffer(GL_TEXTURE_BUFFER, voxel_colors_buf);
		glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(unsigned int) * grid_size, voxel_colors);
	}
	
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_BUFFER, tri_table_txt);

	glBindImageTexture(3, verts_count_txt, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32I);

	shaders->marchingCubesVertsCount->use();
	shaders->marchingCubesVertsCount->setInt("threshold", threshold);

	glDispatchCompute(width - 1, height - 1, depth - 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindImageTexture(0, verts_count_txt, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

	int n = num_cells;
	int k = 10;
	int stride = 1;

	shaders->bufferSum->use();
	shaders->bufferSum->setInt("KERNEL_SIZE", k);
	shaders->bufferSum->setInt("BUFFER_LEN", n);

	int end = ceil(log(n) / log(k));

	for (int i = 0; i < end; i++) {
		shaders->bufferSum->setInt("STRIDE", stride);

		n = n / k + !!(n % k);

		glDispatchCompute(n, 1, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		stride *= k;
	}

	int res = 0;

	glBindBuffer(GL_TEXTURE_BUFFER, verts_count_buf);
	glGetBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(int), &res);

	//printf("RESULT: %d\n", res);

	glBindBuffer(GL_ARRAY_BUFFER, TBO);
	glBufferData(GL_ARRAY_BUFFER, res * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

	shaders->marchingCubes->use();
	shaders->marchingCubes->setInt("threshold", threshold);

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
	glFinish();
	//glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
	//printf("primitives count: %d\n", primitives);
}

void MarchingCubes::draw_mesh(glm::mat4 model)
{
	shaders->renderPass->use();
	shaders->renderPass->setInt("material.diffuse", 0);
	shaders->renderPass->setMat4("model", model);

	glm::mat3 normalMatrix;
	normalMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
	shaders->renderPass->setMat3("normalMatrix", normalMatrix);

	glBindVertexArray(TVAO);
	glDrawTransformFeedback(GL_TRIANGLES, TFO);
	
	glBindVertexArray(0);
}

void MarchingCubes::draw_box(glm::mat4 model)
{
	// visualize bounding box of voxel field
	shaders->line->use();
	shaders->line->setVec4("lineColor", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
	shaders->line->setMat4("model", model);

	glBindVertexArray(boxVAO);
	glDrawArrays(GL_LINES, 0, 24);

	glBindVertexArray(0);
}

void MarchingCubes::bind_voxels_tex_targets()
{
	glBindImageTexture(0, voxels_txt, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16I);
	glBindImageTexture(1, voxel_colors_txt, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
}

void MarchingCubes::setup()
{
	// cells vertex array for the marching cubes geometry shader (executes per each cell)
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(int) * num_cells, cells_verts, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 1, GL_INT/*, GL_FALSE*/, sizeof(int), (void*)0);

	//TODO: probably don't need glActiveTexture(...) during initialization

	// send voxel data as a texture
	glGenBuffers(1, &voxels_buf);
	glGenTextures(1, &voxels_txt);

	glActiveTexture(GL_TEXTURE0);
	glBindBuffer(GL_TEXTURE_BUFFER, voxels_buf);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(short) * grid_size, nullptr, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, voxels_txt);
	//glTexBuffer(GL_TEXTURE_BUFFER, GL_R8UI, buf);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R16I, voxels_buf);

	
	// send voxel color data as a texture
	glGenBuffers(1, &voxel_colors_buf);
	glGenTextures(1, &voxel_colors_txt);

	glActiveTexture(GL_TEXTURE1);
	glBindBuffer(GL_TEXTURE_BUFFER, voxel_colors_buf);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(unsigned int) * grid_size, nullptr, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, voxel_colors_txt);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32UI, voxel_colors_buf);


	// buffer texture used to count how many verts will be generated from marching cubes
	glGenBuffers(1, &verts_count_buf);
	glGenTextures(1, &verts_count_txt);

	glActiveTexture(GL_TEXTURE3);
	glBindBuffer(GL_TEXTURE_BUFFER, verts_count_buf);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(int) * num_cells, nullptr, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, verts_count_txt);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, verts_count_buf);


	// send triangles reference table for marching cubes as a texture
	// Note: this table is too big to include in the shader as a constant
	glGenBuffers(1, &tri_table_buf);
	glGenTextures(1, &tri_table_txt);

	glActiveTexture(GL_TEXTURE2);
	glBindBuffer(GL_TEXTURE_BUFFER, tri_table_buf);
	glBufferData(GL_TEXTURE_BUFFER, 256 * 16 * sizeof(int), triTable, GL_STATIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, tri_table_txt);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, tri_table_buf);


	// bounding box vertices
	glGenVertexArrays(1, &boxVAO);
	glGenBuffers(1, &boxVBO);

	glBindVertexArray(boxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, boxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(boundingBoxVerts), boundingBoxVerts, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);


	// transform feedback of triangle vertices from the geometry construction pass
	glGenVertexArrays(1, &TVAO);
	glGenTransformFeedbacks(1, &TFO);
	glGenBuffers(1, &TBO);

	glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, TFO);
	glBindVertexArray(TVAO);
	glBindBuffer(GL_ARRAY_BUFFER, TBO);
	//glBufferData(GL_ARRAY_BUFFER, num_cells * 15 * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);
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

