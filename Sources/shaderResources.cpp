
#include <shaderResources.hpp>

ShaderResources* ShaderResources::inst = NULL;

ShaderResources* ShaderResources::get_instance()
{
	if (inst == NULL)
		inst = new ShaderResources();
	return inst;
}

void ShaderResources::reset_instance()
{
	delete inst;
	inst = NULL;
}

ShaderResources::ShaderResources()
{
	// regular shader programs (vertex -> frag)
	screenRGBA = new Shader("../KinectSLAM/Shaders/General/screen.vert", "../KinectSLAM/Shaders/General/screenRGBA.frag");
	screenGrey = new Shader("../KinectSLAM/Shaders/General/screen.vert", "../KinectSLAM/Shaders/General/screenGrey.frag");
	screenDepth = new Shader("../KinectSLAM/Shaders/General/screen.vert", "../KinectSLAM/Shaders/Kinect/screenKinectDepth.frag");
	point = new Shader("../KinectSLAM/Shaders/General/point.vert", "../KinectSLAM/Shaders/General/point.frag");
	line = new Shader("../KinectSLAM/Shaders/General/line.vert", "../KinectSLAM/Shaders/General/line.frag");
	texture = new Shader("../KinectSLAM/Shaders/General/texture.vert", "../KinectSLAM/Shaders/General/texture.frag");
	renderPass = new Shader("../KinectSLAM/Shaders/General/renderPass.vert", "../KinectSLAM/Shaders/General/renderPass.frag");
	skybox = new Shader("../KinectSLAM/Shaders/General/skybox.vert", "../KinectSLAM/Shaders/General/skybox.frag");
	kinectPointCloud = new Shader("../KinectSLAM/Shaders/Kinect/kinectPointCloud.vert", "../KinectSLAM/Shaders/Kinect/kinectPointCloud.frag");
	kinectDepthTexture= new Shader("../KinectSLAM/Shaders/General/texture.vert", "../KinectSLAM/Shaders/Kinect/textureKinectDepth.frag");
	
	// shaders including geometry stage (vertex -> geom -> frag)
	normal = new Shader("../KinectSLAM/Shaders/General/normal.vert", "../KinectSLAM/Shaders/General/normal.frag", "../KinectSLAM/Shaders/General/normal.geom");
	// geometry shaders with transform feedback (vertex -> geom) 
	const GLchar* tf_list[] = { "FragPos", "Normal", "Color" };
	marchingCubes = new Shader("../KinectSLAM/Shaders/MarchingCubes/marchingCubes.vert", "../KinectSLAM/Shaders/MarchingCubes/marchingCubes.frag", "../KinectSLAM/Shaders/MarchingCubes/marchingCubes.geom", tf_list);

	// compute shaders 
	// marching cubes 
	marchingCubesVertsCount = new Shader("../KinectSLAM/Shaders/MarchingCubes/marchingCubesVertsCount.comp");
	// voxelization
	kinectVoxelize = new Shader("../KinectSLAM/Shaders/Kinect/kinectVoxelize.comp");
	points2voxels = new Shader("../KinectSLAM/Shaders/Kinect/points2voxels.comp");
	// gpgpu
	bufferSum = new Shader("../KinectSLAM/Shaders/GPGPU/bufferSum.comp");
	// image processing
	rgba2grey = new Shader("../KinectSLAM/Shaders/ImageProcessing/rgba2grey.comp");
	// test shaders
	test = new Shader("../KinectSLAM/Shaders/test.comp");
}

ShaderResources::~ShaderResources()
{
	delete screenRGBA;
	delete screenGrey;
	delete screenDepth;
	delete point;
	delete line;
	delete texture;
	delete normal;
	delete marchingCubes;
	delete marchingCubesVertsCount;
	delete renderPass;
	delete skybox;
	delete kinectPointCloud;
	delete kinectDepthTexture;
	delete kinectVoxelize;
	delete points2voxels;
	delete bufferSum;
	delete rgba2grey;
	delete test;
}

void ShaderResources::update(glm::mat4 view, glm::mat4 projection)
{
	point->use();
	point->setMat4("view", view);
	point->setMat4("projection", projection);

	line->use();
	line->setMat4("view", view);
	line->setMat4("projection", projection);

	texture->use();
	texture->setMat4("view", view);
	texture->setMat4("projection", projection);

	normal->use();
	normal->setMat4("view", view);
	normal->setMat4("projection", projection);

	renderPass->use();
	renderPass->setMat4("view", view);
	renderPass->setMat4("projection", projection);

	skybox->use();
	skybox->setMat4("view", glm::mat4(glm::mat3(view)));
	skybox->setMat4("projection", projection);

	kinectPointCloud->use();
	kinectPointCloud->setMat4("view", view);
	kinectPointCloud->setMat4("projection", projection);

	kinectDepthTexture->use();
	kinectDepthTexture->setMat4("view", view);
	kinectDepthTexture->setMat4("projection", projection);
}