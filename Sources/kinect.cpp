#include <kinect.hpp>

Kinect::Kinect()
{
	if (FAILED(GetDefaultKinectSensor(&sensor)) || !sensor) {
		init_error_flag = true;
		return;
	}
		
	sensor->Open();

	IColorFrameSource* color_framesource = NULL;
	sensor->get_ColorFrameSource(&color_framesource);
	color_framesource->OpenReader(&color_reader);
	color_framesource->get_FrameDescription(&color_description);

	if (color_framesource) {
		color_framesource->Release();
		color_framesource = NULL;
	}

	float vfov_deg = 0.0f;
	float hfov_deg = 0.0f;

	color_description->get_VerticalFieldOfView(&vfov_deg);
	color_description->get_HorizontalFieldOfView(&hfov_deg);
	color_description->get_Width(&color_width);
	color_description->get_Height(&color_height);

	color_frustum.vfov = vfov_deg / 180.0f * PI;
	color_frustum.hfov = hfov_deg / 180.0f * PI;

	IDepthFrameSource* depth_framesource = NULL;
	sensor->get_DepthFrameSource(&depth_framesource);
	depth_framesource->OpenReader(&depth_reader);
	depth_framesource->get_FrameDescription(&depth_description);

	if (depth_framesource) {
		depth_framesource->Release();
		depth_framesource = NULL;
	}

	depth_description->get_Width(&depth_width);
	depth_description->get_Height(&depth_height);
	depth_description->get_VerticalFieldOfView(&vfov_deg);
	depth_description->get_HorizontalFieldOfView(&hfov_deg);

	depth_frustum.vfov = vfov_deg / 180.0f * PI;
	depth_frustum.hfov = hfov_deg / 180.0f * PI;

	shaders = ShaderResources::get_instance();
	mcubes = new MarchingCubes(150, 150, 150);

	float sc = (far_plane - near_plane) * 0.5f;
	mcubes_model = glm::mat4();
	mcubes_model = glm::translate(mcubes_model, glm::vec3(0.0f, 0.0f, -0.5f * (near_plane + far_plane)));
	mcubes_model = glm::scale(mcubes_model, glm::vec3(sc, sc, sc));

	mcubes_model_inv = glm::inverse(mcubes_model);

	createFrustum(&color_frustum, glm::vec4(1, 0, 0, 1));
	createFrustum(&depth_frustum, glm::vec4(1, 1, 0, 1));

	kinect_color_data = (GLubyte*)malloc(4 * color_width * color_height);
	kinect_depth_data = (UINT16*)malloc(2 * depth_width * depth_height);

	createPointCloud();

	// allows vertex shader to manipulate point size
	glEnable(GL_PROGRAM_POINT_SIZE);

	// color camera texture CPU -> GPU
	glGenTextures(1, &kinect_color_texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, color_width, color_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_color_data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// depth camera texture CPU -> GPU
	glGenTextures(1, &kinect_depth_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, depth_width, depth_height, 0, GL_RED, GL_UNSIGNED_SHORT, (GLvoid*)kinect_depth_data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// point cloud output data GPU -> CPU 
	glGenTextures(1, &point_cloud_out_tex);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, point_cloud_out_tex);
	// TODO: check if all these parameters are needed 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, points_width, points_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, point_cloud_out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	shaders->kinectDepthTexture->use();
	shaders->kinectDepthTexture->setInt("tex", 1);
	shaders->kinectDepthTexture->setFloat("far_plane", far_plane);

	shaders->kinectPointCloud->use();
	shaders->kinectPointCloud->setInt("colorTex", 0);
	shaders->kinectPointCloud->setInt("depthTex", 1);
	shaders->kinectPointCloud->setFloat("far_plane", far_plane);
		
	// TODO: move to different spot?
	shaders->kinectPointCloud->setMat4("mcubes_model_inv", mcubes_model_inv);

	shaders->kinectVoxelize->use();
	shaders->kinectVoxelize->setInt("colorTex", 0);
	shaders->kinectVoxelize->setInt("depthTex", 1);
	shaders->kinectVoxelize->setFloat("far_plane", far_plane);

	// TODO: move to different spot?
	shaders->kinectVoxelize->setMat4("mcubes_model_inv", mcubes_model_inv);
	//shaders->kinectVoxelize->setInt("img_output", 2);

	shaders->texture->use();
	shaders->texture->setInt("tex", 2);
}

Kinect::~Kinect()
{
	delete mcubes;
	free(point_cloud_out_buf);
	free(point_cloud_verts);
	free(kinect_color_data);
	free(kinect_depth_data);
	glDeleteVertexArrays(1, &color_frustum.VAO);
	glDeleteVertexArrays(1, &depth_frustum.VAO);
	glDeleteVertexArrays(1, &color_frustum.camVAO);
	glDeleteVertexArrays(1, &depth_frustum.camVAO);
	glDeleteBuffers(1, &color_frustum.VBO);
	glDeleteBuffers(1, &depth_frustum.VBO);
	glDeleteBuffers(1, &color_frustum.camVBO);
	glDeleteBuffers(1, &depth_frustum.camVBO);
}
	
void Kinect::update()
{
	fetchData();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, color_width, color_height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_color_data);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_width, depth_height, GL_RED, GL_UNSIGNED_SHORT, (GLvoid*)kinect_depth_data);
}

void Kinect::draw()
{
	glm::mat4 model;
	//model = glm::scale(model, glm::vec3(10.0f, 10.0f, 10.0f));

	shaders->line->use();
	shaders->line->setMat4("model", model);

	//shaders->line->setVec4("lineColor", color_frustum.color);
	//glBindVertexArray(color_frustum.VAO);
	//glDrawArrays(GL_LINES, 0, 24);

	shaders->line->setVec4("lineColor", depth_frustum.color);
	glBindVertexArray(depth_frustum.VAO);
	glDrawArrays(GL_LINES, 0, 24);

	shaders->texture->use();
	shaders->texture->setMat4("model", model);
	glBindVertexArray(depth_frustum.camVAO);
	//glDrawArrays(GL_TRIANGLES, 0, 6);
	
	shaders->kinectPointCloud->use();
	shaders->kinectPointCloud->setMat4("model", model);
	glPointSize(4.0f);
	glBindVertexArray(pointCloudVAO);
	//glDrawArrays(GL_POINTS, 0, points_width * points_height);

	glBindVertexArray(0);

	mcubes->draw(mcubes_model);
}

void Kinect::createFrustum(Kinect::frustum* f, glm::vec4 color)
{
	f->color = color;

	f->tan_half_vfov = tanf(0.5f * f->vfov);
	f->tan_half_hfov = tanf(0.5f * f->hfov);

	float half_height = near_plane * f->tan_half_vfov;
	float half_width = near_plane * f->tan_half_hfov;

	f->verts[0] = glm::vec3(0.0f, 0.0f, 0.0f);

	f->verts[1] = glm::vec3(-half_width, -half_height, -near_plane);
	f->verts[2] = glm::vec3(half_width, -half_height, -near_plane);
	f->verts[3] = glm::vec3(half_width, half_height, -near_plane);
	f->verts[4] = glm::vec3(-half_width, half_height, -near_plane);

	half_height = far_plane * tanf(f->vfov / 2);
	half_width = far_plane * tanf(f->hfov / 2);

	glm::vec3 a = glm::vec3(-half_width, -half_height, -far_plane);
	glm::vec3 b = glm::vec3(half_width, -half_height, -far_plane);
	glm::vec3 c = glm::vec3(half_width, half_height, -far_plane);
	glm::vec3 d = glm::vec3(-half_width, half_height, -far_plane);

	f->verts[5] = a;
	f->verts[6] = b;
	f->verts[7] = c;
	f->verts[8] = d;

	for (int i = 0; i < 24; i++)
		f->lines[i] = f->verts[frustum_lines_indices[i]];

	f->camera_tex_verts[0].pos = a; f->camera_tex_verts[0].texCoords = glm::vec2(0.0f, 0.0f);
	f->camera_tex_verts[1].pos = b; f->camera_tex_verts[1].texCoords = glm::vec2(1.0f, 0.0f);
	f->camera_tex_verts[2].pos = c; f->camera_tex_verts[2].texCoords = glm::vec2(1.0f, 1.0f);

	f->camera_tex_verts[3].pos = a; f->camera_tex_verts[3].texCoords = glm::vec2(0.0f, 0.0f);
	f->camera_tex_verts[4].pos = c; f->camera_tex_verts[4].texCoords = glm::vec2(1.0f, 1.0f);
	f->camera_tex_verts[5].pos = d; f->camera_tex_verts[5].texCoords = glm::vec2(0.0f, 1.0f);

	glGenVertexArrays(1, &f->VAO);
	glGenBuffers(1, &f->VBO);

	glBindVertexArray(f->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, f->VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(f->lines), f->lines, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	glGenVertexArrays(1, &f->camVAO);
	glGenBuffers(1, &f->camVBO);

	glBindVertexArray(f->camVAO);
	glBindBuffer(GL_ARRAY_BUFFER, f->camVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(f->camera_tex_verts), f->camera_tex_verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(camera_tex_vertex), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(camera_tex_vertex), (void*)(3 * sizeof(float)));
}

void Kinect::createPointCloud()
{
	points_width = depth_width * x_points_per_pix;
	points_height = depth_height * y_points_per_pix;

	shaders->kinectPointCloud->use();

	shaders->kinectPointCloud->setFloat("tan_half_depth_hfov", depth_frustum.tan_half_hfov);
	shaders->kinectPointCloud->setFloat("tan_half_depth_vfov", depth_frustum.tan_half_vfov);
	shaders->kinectPointCloud->setFloat("tan_half_color_hfov", color_frustum.tan_half_hfov);
	shaders->kinectPointCloud->setFloat("tan_half_color_vfov", color_frustum.tan_half_vfov);

	shaders->kinectPointCloud->setVec3("f00", depth_frustum.verts[5]);
	shaders->kinectPointCloud->setVec3("f10", depth_frustum.verts[6]);
	shaders->kinectPointCloud->setVec3("f11", depth_frustum.verts[7]);
	shaders->kinectPointCloud->setVec3("f01", depth_frustum.verts[8]);

	shaders->kinectPointCloud->setInt("points_width", points_width);
	shaders->kinectPointCloud->setInt("points_height", points_height);

	shaders->kinectVoxelize->use();

	shaders->kinectVoxelize->setFloat("tan_half_depth_hfov", depth_frustum.tan_half_hfov);
	shaders->kinectVoxelize->setFloat("tan_half_depth_vfov", depth_frustum.tan_half_vfov);
	shaders->kinectVoxelize->setFloat("tan_half_color_hfov", color_frustum.tan_half_hfov);
	shaders->kinectVoxelize->setFloat("tan_half_color_vfov", color_frustum.tan_half_vfov);

	shaders->kinectVoxelize->setVec3("f00", depth_frustum.verts[5]);
	shaders->kinectVoxelize->setVec3("f10", depth_frustum.verts[6]);
	shaders->kinectVoxelize->setVec3("f11", depth_frustum.verts[7]);
	shaders->kinectVoxelize->setVec3("f01", depth_frustum.verts[8]);

	shaders->kinectVoxelize->setInt("points_width", points_width);
	shaders->kinectVoxelize->setInt("points_height", points_height);

	point_cloud_verts = (glm::ivec2*) malloc(sizeof(glm::ivec2) * points_width * points_height);

	for (int x = 0; x < points_width; x++)
		for (int y = 0; y < points_height; y++) 
			point_cloud_verts[y * points_width + x] = glm::ivec2(x, y);

	point_cloud_out_buf = (glm::vec4*) malloc(sizeof(glm::vec4) * points_width * points_height);
	for (int i = 0; i < points_width * points_height; i++) 
		point_cloud_out_buf[i] = glm::vec4(0, 1, 0, 1);

	glGenVertexArrays(1, &pointCloudVAO);
	glGenBuffers(1, &pointCloudVBO);

	glBindVertexArray(pointCloudVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointCloudVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::ivec2) * points_width * points_height, point_cloud_verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 2, GL_INT, sizeof(glm::ivec2), (void*)0);
}

void Kinect::fetchData()
{
	IColorFrame* color_frame = NULL;
	if (SUCCEEDED(color_reader->AcquireLatestFrame(&color_frame)))
		color_frame->CopyConvertedFrameDataToArray(4 * color_width * color_height, kinect_color_data, ColorImageFormat_Rgba);
	if (color_frame) color_frame->Release();

	IDepthFrame* depth_frame = NULL;
	if (SUCCEEDED(depth_reader->AcquireLatestFrame(&depth_frame)))
		depth_frame->CopyFrameDataToArray(depth_width * depth_height, kinect_depth_data);
	if (depth_frame) depth_frame->Release();
}

void Kinect::debugDump() 
{
	glm::mat4 model;

	shaders->kinectVoxelize->use();
	shaders->kinectVoxelize->setMat4("model", model);

	//glActiveTexture(GL_TEXTURE2);
	//glBindImageTexture(0, point_cloud_out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	glDispatchCompute((GLuint)points_width, (GLuint)points_height, (GLuint)1);

	// TODO: use correct barrier bit
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	shaders->texture->use();
	shaders->texture->setInt("tex", 2);

	glActiveTexture(GL_TEXTURE2);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, point_cloud_out_buf);

	for (int i = 0; i < points_width * points_height; i++) {
		glm::vec3 val = glm::vec3(point_cloud_out_buf[i]);
		glm::ivec3 cell = glm::ivec3(int(val.x * mcubes->width), int(val.y * mcubes->height), int(val.z * mcubes->depth));
		
		mcubes->set_voxel(cell.x, cell.y, cell.z, 50);
		mcubes->set_voxel(cell.x + 1, cell.y, cell.z, 50);
		mcubes->set_voxel(cell.x + 1, cell.y + 1, cell.z, 50);
		mcubes->set_voxel(cell.x, cell.y + 1, cell.z, 50);

		mcubes->set_voxel(cell.x, cell.y, cell.z + 1, 50);
		mcubes->set_voxel(cell.x + 1, cell.y, cell.z + 1, 50);
		mcubes->set_voxel(cell.x + 1, cell.y + 1, cell.z + 1, 50);
		mcubes->set_voxel(cell.x, cell.y + 1, cell.z + 1, 50);
	}

	mcubes->updateGeometry(25, true);
	/*
	printf("\nDEBUG DUMP STARTED:\n\n");
	for (int i = 0; i < 50; i++) {
		glm::vec4 val = point_cloud_out_buf[points_width * i + i];
		printf("\t(%f, %f, %f, %f)\n", val.r, val.g, val.b, val.a);
	}
	printf("\nDEBUG DUMP ENDED\n");
	*/
	//exit(0);
}
