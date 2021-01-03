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
	mcubes = new MarchingCubes(voxels_resolution, voxels_resolution, voxels_resolution);

	voxels_box_scale = (far_plane - near_plane);
	voxels_box_translation = glm::vec3(0.0f, 0.0f, -0.5f * (1.0f * near_plane + 1.0f * far_plane));

	createFrustum(&color_frustum, glm::vec4(1, 0, 0, 1));
	createFrustum(&depth_frustum, glm::vec4(1, 1, 0, 1));

	kinect_color_data = (GLubyte*)malloc(4 * color_width * color_height);
	kinect_depth_data = (UINT16*)malloc(2 * depth_width * depth_height);

	createPointCloud();

	// allows vertex shader to manipulate point size
	glEnable(GL_PROGRAM_POINT_SIZE);

	initKinectTextures();
	initVoxelizationTextures();
	initImageProcessing();

	// TODO: far plane needs to be in a different spot 
	shaders->kinectDepthTexture->use();
	shaders->kinectDepthTexture->setFloat("far_plane", far_plane);

	shaders->kinectPointCloud->use();
	shaders->kinectPointCloud->setFloat("far_plane", far_plane);

	shaders->kinectPointCloudFiltered->use();
	shaders->kinectPointCloudFiltered->setFloat("far_plane", far_plane);

	shaders->screenDepth->use();
	shaders->screenDepth->setFloat("far_plane", far_plane);
		
	// TODO: move to different spot?
	shaders->kinectBuckets->use();
	shaders->kinectBuckets->setFloat("far_plane", far_plane);

	shaders->kinectBuckets->setInt("buckets_width", buckets_width);

	shaders->kinectSavePoints->use();
	shaders->kinectSavePoints->setFloat("far_plane", far_plane);
}

Kinect::~Kinect()
{
	delete mcubes;
	free(point_cloud_out_data);
	free(point_cloud_verts);
	free(kinect_color_data);
	free(kinect_depth_data);
	free(buckets_heads);
	free(buckets_nodes);
	glDeleteVertexArrays(1, &color_frustum.VAO);
	glDeleteVertexArrays(1, &depth_frustum.VAO);
	glDeleteVertexArrays(1, &color_frustum.camVAO);
	glDeleteVertexArrays(1, &depth_frustum.camVAO);
	glDeleteTextures(1, &kinect_color_texture);
	glDeleteTextures(1, &kinect_depth_texture);
	glDeleteTextures(1, &point_cloud_out_tex);
	glDeleteBuffers(1, &color_frustum.VBO);
	glDeleteBuffers(1, &depth_frustum.VBO);
	glDeleteBuffers(1, &color_frustum.camVBO);
	glDeleteBuffers(1, &depth_frustum.camVBO);
}
	
bool Kinect::get_init_error()
{
	return init_error_flag;
}

void Kinect::updateImageProcessing()
{
	// convert to grayscale
	glBindImageTexture(0, kinect_color_texture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8UI);
	glBindImageTexture(1, grey_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

	shaders->rgba2grey->use();

	glDispatchCompute(color_width, color_height, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// run edge detector
	glBindImageTexture(0, grey_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
	glBindImageTexture(1, edges_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

	shaders->edges->use();

	glDispatchCompute(color_width - 2, color_height - 2, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// threshold image 
	glBindImageTexture(0, edges_tex, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8UI);
	glBindImageTexture(1, edges_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R8UI);

	shaders->threshold->use();
	shaders->threshold->setUnsigned("threshold", 80);

	glDispatchCompute(color_width - 2, color_height - 2, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void Kinect::update()
{
	fetchData();

	glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_width, depth_height, GL_RED, GL_UNSIGNED_SHORT, (GLvoid*)kinect_depth_data);

	glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, color_width, color_height, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_color_data);
	
	updateImageProcessing();
	updateInput();
}

void Kinect::updateInput() {
	if (create_mesh) {
		createMesh();
		create_mesh = false;
	}

	if (save_points) {
		savePoints();
		save_points = false;
	}

	if (clear_points) {
		clear();
		clear_points = false;
	}
}

void Kinect::draw()
{
	model = glm::mat4();
	model = glm::scale(model, glm::vec3(render_scale));

	mcubes_model = glm::mat4();
	mcubes_model = glm::scale(mcubes_model, glm::vec3(render_scale));
	mcubes_model = glm::translate(mcubes_model, voxels_box_translation);
	mcubes_model = glm::scale(mcubes_model, glm::vec3(voxels_box_scale));

	mcubes_model_inv = glm::inverse(mcubes_model);

	if (render_screen_color) {
		//ScreenQuad::draw(shaders->screenGrey, edges_tex);
		ScreenQuad::draw(shaders->screenRGBA, kinect_color_texture);
		return;
	}

	if (render_screen_depth) {
		ScreenQuad::draw(shaders->screenDepth, kinect_depth_texture);
		return;
	}

	shaders->line->use();
	shaders->line->setMat4("model", model);

	if (render_frustum) {
		shaders->line->setVec4("lineColor", depth_frustum.color);
		glBindVertexArray(depth_frustum.VAO);
		glDrawArrays(GL_LINES, 0, 24);
	}

	if (render_depth_txt) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
		shaders->kinectDepthTexture->use();
		shaders->kinectDepthTexture->setMat4("model", model);
		glBindVertexArray(depth_frustum.camVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	if (1 & render_point_cloud) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
		shaders->kinectPointCloud->use();
		shaders->kinectPointCloud->setMat4("model", model);
		shaders->kinectPointCloud->setMat4("mcubes_model_inv", mcubes_model_inv);
		glPointSize(4.0f);
		glBindVertexArray(pointCloudVAO);
		glDrawArrays(GL_POINTS, 0, points_width * points_height);
	}

	// TODO: remove tmp
	if (0 & render_point_cloud) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, edges_tex);
		shaders->kinectPointCloudFiltered->use();
		shaders->kinectPointCloudFiltered->setMat4("model", model);
		shaders->kinectPointCloudFiltered->setMat4("mcubes_model_inv", mcubes_model_inv);
		glPointSize(4.0f);
		glBindVertexArray(pointCloudVAO);
		glDrawArrays(GL_POINTS, 0, points_width * points_height);
	}

	glBindVertexArray(0);

	// add render flag for this
	registration->draw();

	if (render_mesh) 
		mcubes->draw_mesh(mcubes_model);

	if (render_voxels_box)
		mcubes->draw_box(mcubes_model);
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

	//TODO: find way to reduce duplicate code 

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

	shaders->kinectPointCloudFiltered->use();

	shaders->kinectPointCloudFiltered->setFloat("tan_half_depth_hfov", depth_frustum.tan_half_hfov);
	shaders->kinectPointCloudFiltered->setFloat("tan_half_depth_vfov", depth_frustum.tan_half_vfov);
	shaders->kinectPointCloudFiltered->setFloat("tan_half_color_hfov", color_frustum.tan_half_hfov);
	shaders->kinectPointCloudFiltered->setFloat("tan_half_color_vfov", color_frustum.tan_half_vfov);

	shaders->kinectPointCloudFiltered->setVec3("f00", depth_frustum.verts[5]);
	shaders->kinectPointCloudFiltered->setVec3("f10", depth_frustum.verts[6]);
	shaders->kinectPointCloudFiltered->setVec3("f11", depth_frustum.verts[7]);
	shaders->kinectPointCloudFiltered->setVec3("f01", depth_frustum.verts[8]);

	shaders->kinectPointCloudFiltered->setInt("points_width", points_width);
	shaders->kinectPointCloudFiltered->setInt("points_height", points_height);

	shaders->kinectBuckets->use();

	shaders->kinectBuckets->setFloat("tan_half_depth_hfov", depth_frustum.tan_half_hfov);
	shaders->kinectBuckets->setFloat("tan_half_depth_vfov", depth_frustum.tan_half_vfov);
	shaders->kinectBuckets->setFloat("tan_half_color_hfov", color_frustum.tan_half_hfov);
	shaders->kinectBuckets->setFloat("tan_half_color_vfov", color_frustum.tan_half_vfov);

	shaders->kinectBuckets->setVec3("f00", depth_frustum.verts[5]);
	shaders->kinectBuckets->setVec3("f10", depth_frustum.verts[6]);
	shaders->kinectBuckets->setVec3("f11", depth_frustum.verts[7]);
	shaders->kinectBuckets->setVec3("f01", depth_frustum.verts[8]);

	shaders->kinectBuckets->setInt("points_width", points_width);
	shaders->kinectBuckets->setInt("points_height", points_height);

	shaders->kinectSavePoints->use();

	shaders->kinectSavePoints->setFloat("tan_half_depth_hfov", depth_frustum.tan_half_hfov);
	shaders->kinectSavePoints->setFloat("tan_half_depth_vfov", depth_frustum.tan_half_vfov);
	shaders->kinectSavePoints->setFloat("tan_half_color_hfov", color_frustum.tan_half_hfov);
	shaders->kinectSavePoints->setFloat("tan_half_color_vfov", color_frustum.tan_half_vfov);

	shaders->kinectSavePoints->setVec3("f00", depth_frustum.verts[5]);
	shaders->kinectSavePoints->setVec3("f10", depth_frustum.verts[6]);
	shaders->kinectSavePoints->setVec3("f11", depth_frustum.verts[7]);
	shaders->kinectSavePoints->setVec3("f01", depth_frustum.verts[8]);

	shaders->kinectSavePoints->setInt("points_width", points_width);
	shaders->kinectSavePoints->setInt("points_height", points_height);

	registration = new PointCloudRegistration(points_width * points_height);

	point_cloud_out_data = (glm::vec4*) malloc(sizeof(glm::vec4) * 2 * points_width * points_height);
	
	buckets_heads = (int*)malloc(sizeof(int) * buckets_size);
	buckets_nodes = (int*)malloc(sizeof(int) * points_width * points_height);

	point_cloud_verts = (glm::ivec2*) malloc(sizeof(glm::ivec2) * points_width * points_height);

	for (int x = 0; x < points_width; x++)
		for (int y = 0; y < points_height; y++) 
			point_cloud_verts[y * points_width + x] = glm::ivec2(x, y);

	glGenVertexArrays(1, &pointCloudVAO);
	glGenBuffers(1, &pointCloudVBO);

	glBindVertexArray(pointCloudVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointCloudVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::ivec2) * points_width * points_height, point_cloud_verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribIPointer(0, 2, GL_INT, sizeof(glm::ivec2), (void*)0);
}

void Kinect::initKinectTextures()
{
	// color camera texture CPU -> GPU
	glGenTextures(1, &kinect_color_texture);
	glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, color_width, color_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_color_data);
	glBindTexture(GL_TEXTURE_2D, 0);

	// depth camera texture CPU -> GPU
	glGenTextures(1, &kinect_depth_texture);
	glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, depth_width, depth_height, 0, GL_RED, GL_UNSIGNED_SHORT, (GLvoid*)kinect_depth_data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Kinect::initVoxelizationTextures()
{
	// point cloud output data GPU -> CPU 
	glGenTextures(1, &point_cloud_out_tex);
	glBindTexture(GL_TEXTURE_2D, point_cloud_out_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2 * points_width, points_height, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// list of linked lists (just the index to head) representing points that landed in each cell [-1 represents no next]
	glGenBuffers(1, &buckets_heads_buf);
	glGenTextures(1, &buckets_heads_tex);
	glBindBuffer(GL_TEXTURE_BUFFER, buckets_heads_buf);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(int) * buckets_size, nullptr, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, buckets_heads_tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, buckets_heads_buf);
	glBindTexture(GL_TEXTURE_BUFFER, 0);

	// the point cloud nodes (just the index to next) belonging to the linked lists for each cell [-1 represents no next]
	glGenBuffers(1, &buckets_nodes_buf);
	glGenTextures(1, &buckets_nodes_tex);
	glBindBuffer(GL_TEXTURE_BUFFER, buckets_nodes_buf);
	glBufferData(GL_TEXTURE_BUFFER, sizeof(int) * points_width * points_height, nullptr, GL_DYNAMIC_DRAW);
	glBindTexture(GL_TEXTURE_BUFFER, buckets_nodes_tex);
	glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, buckets_nodes_buf);
	glBindTexture(GL_TEXTURE_BUFFER, 0);
}

void Kinect::initImageProcessing()
{
	// grey texture (GPU only)
	glGenTextures(1, &grey_tex);
	glBindTexture(GL_TEXTURE_2D, grey_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, color_width, color_height, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	// edges texture (GPU only)
	glGenTextures(1, &edges_tex);
	glBindTexture(GL_TEXTURE_2D, edges_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, color_width - 2, color_height - 2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
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

void Kinect::createMesh()
{
	viewMesh();

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kinect_color_texture);

	glBindImageTexture(2, point_cloud_out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	shaders->kinectBuckets->use();
	shaders->kinectBuckets->setMat4("model", model);
	shaders->kinectBuckets->setMat4("buckets_model_inv", mcubes_model_inv);

	glDispatchCompute(points_width, points_height, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindTexture(GL_TEXTURE_2D, point_cloud_out_tex);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, point_cloud_out_data);

	// fill voxel buckets, each bucket is a linked list representing the points from the point cloud that landed in that cell

	// reset voxel buckets linked lists
	memset(buckets_heads, 0xff, sizeof(int) * buckets_size);
	memset(buckets_nodes, 0xff, sizeof(int) * points_width * points_height);
	for (int i = 0; i < points_width * points_height; i++) {
		glm::ivec3 cell = point_cloud_out_data[2 * i];
		cell = glm::ivec3(cell.x, cell.y, cell.z);

		if (cell.x < 0 || cell.y < 0 || cell.z < 0 ||
			cell.x >= buckets_width || cell.y >= buckets_width || cell.z >= buckets_width)
			continue;

		int cell_index = buckets_width * buckets_width * cell.z + buckets_width * cell.y + cell.x;
		// put the current head of the cell list as the next for this point node 
		buckets_nodes[i] = buckets_heads[cell_index];
		// put this point node as the new head of the cell list
		buckets_heads[cell_index] = i;
	}

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_BUFFER, buckets_heads_tex);
	glBindBuffer(GL_TEXTURE_BUFFER, buckets_heads_buf);
	glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(int) * buckets_size, buckets_heads);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_BUFFER, buckets_nodes_tex);
	glBindBuffer(GL_TEXTURE_BUFFER, buckets_nodes_buf);
	glBufferSubData(GL_TEXTURE_BUFFER, 0, sizeof(int) * points_width * points_height, buckets_nodes);

	mcubes->bind_voxels_tex_targets();

	shaders->buckets2voxels->use();
	shaders->buckets2voxels->setInt("POINTS_WIDTH", points_width);
	shaders->buckets2voxels->setInt("POINTS_HEIGHT", points_height);
	shaders->buckets2voxels->setInt("BUCKETS_WIDTH", buckets_width);
	shaders->buckets2voxels->setInt("WIDTH", mcubes->width);
	shaders->buckets2voxels->setInt("HEIGHT", mcubes->height);
	shaders->buckets2voxels->setInt("DEPTH", mcubes->depth);

	glDispatchCompute(mcubes->width, mcubes->height, mcubes->depth);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	mcubes->updateGeometry(20, false);
}

void Kinect::savePoints() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, kinect_color_texture);

	glBindImageTexture(2, point_cloud_out_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	shaders->kinectSavePoints->use();
	shaders->kinectSavePoints->setMat4("model", model);

	glDispatchCompute(points_width, points_height, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glBindTexture(GL_TEXTURE_2D, point_cloud_out_tex);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, point_cloud_out_data);

	if (registration->getModelSet()) {
		registration->registerToModel(point_cloud_out_data);
		render_point_cloud = false;
	} 
	else
		registration->setModel(point_cloud_out_data, mcubes_model);
}

void Kinect::clear() {
	registration->clear();
}

void Kinect::viewPointCloud()
{
	render_mesh = false;
	render_point_cloud = true;
}

void Kinect::viewMesh()
{
	render_mesh = true;
	render_point_cloud = false;
}

void Kinect::setInputFlag(KinectInputFlag flag) {
	switch (flag) {
		case KinectInputFlag::CREATE_MESH:
			create_mesh = true;
		break;
		case KinectInputFlag::SAVE_POINTS:
			save_points = true;
		break;
		case KinectInputFlag::CLEAR:
			clear_points = true;
		break;
	}
}
