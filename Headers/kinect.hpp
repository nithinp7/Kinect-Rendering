#ifndef KIN_H
#define KIN_H

#include <Kinect.h>

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <shader.hpp>
#include <shaderResources.hpp>
#include <resourcesLoader.hpp>

#include <marchingCubes.hpp>

#include <screenQuad.hpp>

#include <math.h>

#define PI 3.14159265358979323846  /* pi */
#define EPSILON 1e-4f

class Kinect {
	 
public:

	// public render flags
	bool render_point_cloud = true;
	bool render_mesh = false;
	bool render_frustum = true;
	bool render_voxels_box = true;
	bool render_depth_txt = false;
	bool render_color_txt = false;
	bool render_screen_depth = false;
	bool render_screen_color = false;

	// voxels box alignment to point cloud
	float voxels_box_scale;
	glm::vec3 voxels_box_translation;

	Kinect();
	~Kinect();
	bool get_init_error();
	void update();
	void draw();
	//dbg
	void createMeshGPU();
	void createMeshCPU();
	void viewPointCloud();
	void viewMesh();

private:

	// read only initialization error flag
	bool init_error_flag = false;

	// render paramaters 
	glm::mat4 model;
	const float render_scale = 10.0f;

	// global shader manager
	ShaderResources* shaders;

	// custom compute shader, creates point cloud and transforms it into voxel space
	GLuint kinectVoxelizeShader;

	// marching cubes and voxelization related
	MarchingCubes* mcubes;
	glm::mat4 mcubes_model;
	glm::mat4 mcubes_model_inv;
	const int voxels_resolution = 200;

	// list of linked lists (just the index to head) representing points that landed in each cell [-1 represents no next]
	int* cell_buckets_heads;
	GLuint cell_buckets_heads_tex;
	GLuint cell_buckets_heads_buf;
	// the point cloud nodes (just the index to next) belonging to the linked lists for each cell [-1 represents no next]
	int* cell_buckets_nodes;
	GLuint cell_buckets_nodes_tex;
	GLuint cell_buckets_nodes_buf;

	// camera textures
	struct camera_tex_vertex {
		glm::vec3 pos;
		glm::vec2 texCoords;
	};

	struct frustum {
		float vfov;
		float tan_half_vfov;
		float hfov;
		float tan_half_hfov;
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
	glm::ivec2* point_cloud_verts;
	glm::vec4* point_cloud_out_data;
	GLuint point_cloud_out_buf;
	GLuint point_cloud_out_tex;
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