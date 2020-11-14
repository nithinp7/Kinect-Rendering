
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
	screen = new Shader("../KinectSLAM/Shaders/screenShader.vert", "../KinectSLAM/Shaders/screenShader.frag");
	point = new Shader("../KinectSLAM/Shaders/point.vert", "../KinectSLAM/Shaders/point.frag");
	line= new Shader("../KinectSLAM/Shaders/line.vert", "../KinectSLAM/Shaders/line.frag");
	texture = new Shader("../KinectSLAM/Shaders/texture.vert", "../KinectSLAM/Shaders/texture.frag");
	normal = new Shader("../KinectSLAM/Shaders/normal.vert", "../KinectSLAM/Shaders/normal.frag", "../KinectSLAM/Shaders/normal.geom");
	marchingCubes = new Shader("../KinectSLAM/Shaders/marchingCubes.vert", "../KinectSLAM/Shaders/marchingCubes.frag", "../KinectSLAM/Shaders/marchingCubes.geom", true);
	renderPass = new Shader("../KinectSLAM/Shaders/renderPass.vert", "../KinectSLAM/Shaders/renderPass.frag");
	skybox = new Shader("../KinectSLAM/Shaders/skybox.vert", "../KinectSLAM/Shaders/skybox.frag");
	kinectPointCloud = new Shader("../KinectSLAM/Shaders/kinectPointCloud.vert", "../KinectSLAM/Shaders/kinectPointCloud.frag");
	kinectDepthTexture= new Shader("../KinectSLAM/Shaders/kinectDepthTexture.vert", "../KinectSLAM/Shaders/kinectDepthTexture.frag");
}

ShaderResources::~ShaderResources()
{
	delete screen;
	delete point;
	delete line;
	delete texture;
	delete normal;
	delete marchingCubes;
	delete renderPass;
	delete skybox;
	delete kinectPointCloud;
	delete kinectDepthTexture;
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