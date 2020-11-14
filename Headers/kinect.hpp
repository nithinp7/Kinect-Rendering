#ifndef KIN_H
#define KIN_H

#include <Kinect.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.hpp>
#include <shaderResources.hpp>

#include <math.h>

#define PI 3.14159265358979323846  /* pi */

class Kinect {
	 
public:

	bool init_error_flag = false;

	Kinect();
	~Kinect();
	void update();
	void draw();

private:
	// global shader manager
	ShaderResources* shaders;

	// camera textures
	struct camera_tex_vertex {
		glm::vec3 pos;
		glm::vec2 texCoords;
	};

	struct frustum {
		float vfov;
		float hfov;
		glm::vec3 verts[9];
		glm::vec3 lines[24];
		glm::vec4 color;
		unsigned int VAO;
		unsigned int VBO;

		camera_tex_vertex camera_tex_verts[6];
		unsigned int camVAO;
		unsigned int camVBO;
	};

	const int frustum_lines_indices[24] = {
		0, 5,
		0, 6,
		0, 7,
		0, 8,

		1, 2,
		2, 3,
		3, 4,
		4, 1,

		5, 6,
		6, 7,
		7, 8,
		8, 5
	};

	IKinectSensor* sensor;

	// color camera
	IColorFrameReader* color_reader;
	IFrameDescription* color_description;
	GLuint kinect_color_texture;
	GLubyte* kinect_color_data;
	int color_width = -1;
	int color_height = -1;
	frustum color_frustum;

	// depth camera
	const float near_plane = 0.4f;
	const float far_plane = 4.0f;
	IDepthFrameReader* depth_reader;
	IFrameDescription* depth_description;
	GLuint kinect_depth_texture;
	UINT16* kinect_depth_data;
	int depth_width = -1;
	int depth_height = -1;
	frustum depth_frustum;

	// point cloud
	glm::vec2* point_cloud_verts;
	int points_width = -1;
	int points_height = -1;
	const int x_points_per_pix = 5;
	const int y_points_per_pix = 5;
	unsigned int pointCloudVAO;
	unsigned int pointCloudVBO;

	void createFrustum(frustum* f, glm::vec4 color);
	void createPointCloud();
	void fetchData();
};

#endif