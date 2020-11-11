#ifndef SHADER_RESOURCES_H
#define SHADER_RESOURCES_H

#include <shader.hpp>

Shader* screenShader;
Shader* pointShader;
Shader* lineShader;
Shader* normalShader;
Shader* textureShader;
Shader* marchingCubesShader;
Shader* renderPassShader;
Shader* skyboxShader;
Shader* kinectPointCloudShader;
Shader* kinectDepthTextureShader;

void initShaders()
{
	screenShader = new Shader("../KinectSLAM/Shaders/screenShader.vert", "../KinectSLAM/Shaders/screenShader.frag");
	pointShader = new Shader("../KinectSLAM/Shaders/point.vert", "../KinectSLAM/Shaders/point.frag");
	lineShader = new Shader("../KinectSLAM/Shaders/line.vert", "../KinectSLAM/Shaders/line.frag");
	textureShader = new Shader("../KinectSLAM/Shaders/texture.vert", "../KinectSLAM/Shaders/texture.frag");
	normalShader = new Shader("../KinectSLAM/Shaders/normal.vert", "../KinectSLAM/Shaders/normal.frag", "../KinectSLAM/Shaders/normal.geom");
	marchingCubesShader = new Shader("../KinectSLAM/Shaders/marchingCubes.vert", "../KinectSLAM/Shaders/marchingCubes.frag", "../KinectSLAM/Shaders/marchingCubes.geom", true);
	renderPassShader = new Shader("../KinectSLAM/Shaders/renderPass.vert", "../KinectSLAM/Shaders/renderPass.frag");
	skyboxShader = new Shader("../KinectSLAM/Shaders/skybox.vert", "../KinectSLAM/Shaders/skybox.frag");
	kinectPointCloudShader = new Shader("../KinectSLAM/Shaders/kinectPointCloud.vert", "../KinectSLAM/Shaders/kinectPointCloud.frag");
	kinectDepthTextureShader = new Shader("../KinectSLAM/Shaders/kinectDepthTexture.vert", "../KinectSLAM/Shaders/kinectDepthTexture.frag");
}

void updateShaders(glm::mat4 view, glm::mat4 projection)
{
	pointShader->use();
	pointShader->setMat4("view", view);
	pointShader->setMat4("projection", projection);

	lineShader->use();
	lineShader->setMat4("view", view);
	lineShader->setMat4("projection", projection);
	
	textureShader->use();
	textureShader->setMat4("view", view);
	textureShader->setMat4("projection", projection);

	normalShader->use();
	normalShader->setMat4("view", view);
	normalShader->setMat4("projection", projection);

	renderPassShader->use();
	renderPassShader->setMat4("view", view);
	renderPassShader->setMat4("projection", projection);

	skyboxShader->use();
	skyboxShader->setMat4("view", glm::mat4(glm::mat3(view)));
	skyboxShader->setMat4("projection", projection);

	kinectPointCloudShader->use();
	kinectPointCloudShader->setMat4("view", view);
	kinectPointCloudShader->setMat4("projection", projection);

	kinectDepthTextureShader->use();
	kinectDepthTextureShader->setMat4("view", view);
	kinectDepthTextureShader->setMat4("projection", projection);
}

void deleteShaders()
{
	delete screenShader;
	delete pointShader;
	delete lineShader;
	delete textureShader;
	delete normalShader;
	delete marchingCubesShader;
	delete renderPassShader;
	delete skyboxShader;
	delete kinectPointCloudShader;
	delete kinectDepthTextureShader;
}

#endif