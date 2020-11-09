#ifndef KIN_H
#define KIN_H

#include <Kinect.h>

Shader* screenShader;

const unsigned int KINECT_WIDTH = 1920;
const unsigned int KINECT_HEIGHT = 1080;
IKinectSensor* sensor;
IColorFrameReader* reader;
GLuint kinect_texture;
GLubyte kinect_data[KINECT_WIDTH * KINECT_HEIGHT * 4];

bool initKinect()
{
	if (FAILED(GetDefaultKinectSensor(&sensor)))
		return false;

	if (sensor) {
		sensor->Open();

		IColorFrameSource* framesource = NULL;
		sensor->get_ColorFrameSource(&framesource);
		framesource->OpenReader(&reader);

		if (framesource) {
			framesource->Release();
			framesource = NULL;
		}

		screenShader = new Shader("../KinectSLAM/Shaders/screenShader.vert", "../KinectSLAM/Shaders/screenShader.frag");

		screenShader->use();
		screenShader->setInt("screenTexture", 0);

		glGenTextures(1, &kinect_texture);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, kinect_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, KINECT_WIDTH, KINECT_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_data);
		glBindTexture(GL_TEXTURE_2D, 0);

		return true;
	}
	else
		return false;
}

void getKinectData() 
{
	IColorFrame* frame = NULL;
	if (SUCCEEDED(reader->AcquireLatestFrame(&frame)))
		frame->CopyConvertedFrameDataToArray(KINECT_WIDTH * KINECT_HEIGHT * 4, kinect_data, ColorImageFormat_Rgba);
	if (frame) frame->Release();
}

void updateKinect() 
{
	getKinectData();
	glBindTexture(GL_TEXTURE_2D, kinect_texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, KINECT_WIDTH, KINECT_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)kinect_data);
}

void drawKinect()
{
	screenShader->use();
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);
}

void deleteKinect() {
	delete screenShader;
}

#endif