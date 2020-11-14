#include <kinect.hpp>

Kinect::Kinect()
{
	if (FAILED(GetDefaultKinectSensor(&sensor)))
		init_error_flag = true;
	else if (sensor) {
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

		createFrustum(&color_frustum, glm::vec4(1, 0, 0, 1));
		createFrustum(&depth_frustum, glm::vec4(1, 1, 0, 1));

		kinect_color_data = (GLubyte*)malloc(4 * color_width * color_height);
		kinect_depth_data = (UINT16*)malloc(2 * depth_width * depth_height);

		createPointCloud();

		// allows vertex shader to manipulate point size
		glEnable(GL_PROGRAM_POINT_SIZE);

		glGenTextures(1, &kinect_color_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kinect_color_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, color_width, color_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_color_data);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenTextures(1, &kinect_depth_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, kinect_depth_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R16, depth_width, depth_height, 0, GL_RED, GL_UNSIGNED_SHORT, (GLvoid*)kinect_depth_data);
		glBindTexture(GL_TEXTURE_2D, 0);

		shaders->kinectDepthTexture->use();
		shaders->kinectDepthTexture->setInt("tex", 1);
		shaders->kinectDepthTexture->setFloat("far_plane", far_plane);

		shaders->kinectPointCloud->use();
		shaders->kinectPointCloud->setInt("colorTex", 0);
		shaders->kinectPointCloud->setInt("depthTex", 1);
		shaders->kinectPointCloud->setFloat("far_plane", far_plane);
	}
	else
		init_error_flag = true;
}

Kinect::~Kinect()
{
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
	/* * /
	shaders->screen->use();
	shaders->screen->setInt("screenTexture", 0);
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
	/**/

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

	/*
	shaders->kinectDepthTexture->use();
	shaders->kinectDepthTexture->setMat4("model", model);
	glBindVertexArray(depth_frustum.camVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	*/

	shaders->kinectPointCloud->use();
	shaders->kinectPointCloud->setMat4("model", model);
	//glPointSize(4.0f);
	glBindVertexArray(pointCloudVAO);
	glDrawArrays(GL_POINTS, 0, points_width * points_height);

	glBindVertexArray(0);
}

void Kinect::createFrustum(frustum* f, glm::vec4 color)
{
	f->color = color;

	float half_height = near_plane * tanf(0.5f * f->vfov);
	float half_width = near_plane * tanf(0.5f * f->hfov);

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
	shaders->kinectPointCloud->use();

	shaders->kinectPointCloud->setFloat("tan_half_depth_hfov", tanf(0.5f * depth_frustum.hfov));
	shaders->kinectPointCloud->setFloat("tan_half_depth_vfov", tanf(0.5f * depth_frustum.vfov));
	shaders->kinectPointCloud->setFloat("tan_half_color_hfov", tanf(0.5f * color_frustum.hfov));
	shaders->kinectPointCloud->setFloat("tan_half_color_vfov", tanf(0.5f * color_frustum.vfov));

	shaders->kinectPointCloud->setVec3("f00", depth_frustum.verts[5]);
	shaders->kinectPointCloud->setVec3("f10", depth_frustum.verts[6]);
	shaders->kinectPointCloud->setVec3("f11", depth_frustum.verts[7]);
	shaders->kinectPointCloud->setVec3("f01", depth_frustum.verts[8]);

	points_width = depth_width * x_points_per_pix;
	points_height = depth_height * y_points_per_pix;

	point_cloud_verts = (glm::vec2*) malloc(sizeof(glm::vec2) * points_width * points_height);

	for (int x = 0; x < points_width; x++)
		for (int y = 0; y < points_height; y++)
			point_cloud_verts[y * points_width + x] = glm::vec2((float)x / points_width, (float)y / points_height);

	glGenVertexArrays(1, &pointCloudVAO);
	glGenBuffers(1, &pointCloudVBO);

	glBindVertexArray(pointCloudVAO);
	glBindBuffer(GL_ARRAY_BUFFER, pointCloudVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * points_width * points_height, point_cloud_verts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
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
