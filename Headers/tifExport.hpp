#ifndef TIF_EXP_H
#define TIF_EXP_H

#include <glad/glad.h>

#include <iostream>

unsigned int printID = 1;
bool print = false;

const unsigned int HEADER_SIZE = 8;
const unsigned int STRIP_SIZE = SCR_WIDTH * SCR_HEIGHT * 3;
const unsigned int IFD_SIZE = 174;
const unsigned int IFD_OFFS = STRIP_SIZE + HEADER_SIZE;
const unsigned int EXTRA_OFFS = IFD_OFFS + IFD_SIZE;
const unsigned int EXTRA_0 = EXTRA_OFFS;
const unsigned int EXTRA_1 = EXTRA_OFFS + 6;
const unsigned int EXTRA_2 = EXTRA_OFFS + 12;
const unsigned int EXTRA_3 = EXTRA_OFFS + 18;

const char TIF_HEADER[] = { 'M', 'M', 0x00, 0x2a, IFD_OFFS >> 24, IFD_OFFS >> 16 & 0xff, IFD_OFFS >> 8 & 0xff, IFD_OFFS & 0xff };
const char TIF_FOOTER[] = {
	// IFD begin
	0x00, 0x0e,

	// IFD entries
	0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, SCR_WIDTH >> 24, SCR_WIDTH >> 16 & 0xff, SCR_WIDTH >> 8 & 0xff, SCR_WIDTH & 0xff,
	0x01, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, SCR_HEIGHT >> 24, SCR_HEIGHT >> 16 & 0xff, SCR_HEIGHT >> 8 & 0xff, SCR_HEIGHT & 0xff,
	0x01, 0x02, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, EXTRA_0 >> 24, EXTRA_0 >> 16 & 0xff, EXTRA_0 >> 8 & 0xff, EXTRA_0 & 0xff,
	0x01, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
	0x01, 0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00,
	0x01, 0x11, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08,
	0x01, 0x12, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
	0x01, 0x15, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x00, 0x00,
	0x01, 0x16, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, SCR_HEIGHT >> 24, SCR_HEIGHT >> 16 & 0xff, SCR_HEIGHT >> 8 & 0xff, SCR_HEIGHT & 0xff,
	0x01, 0x17, 0x00, 0x04, 0x00, 0x00, 0x00, 0x01, STRIP_SIZE >> 24, STRIP_SIZE >> 16 & 0xff, STRIP_SIZE >> 8 & 0xff, STRIP_SIZE & 0xff,
	0x01, 0x18, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, EXTRA_1 >> 24, EXTRA_1 >> 16 & 0xff, EXTRA_1 >> 8 & 0xff, EXTRA_1 & 0xff,
	0x01, 0x19, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, EXTRA_2 >> 24, EXTRA_2 >> 16 & 0xff, EXTRA_2 >> 8 & 0xff, EXTRA_2 & 0xff,
	0x01, 0x1c, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
	0x01, 0x53, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, EXTRA_3 >> 24, EXTRA_3 >> 16 & 0xff, EXTRA_3 >> 8 & 0xff, EXTRA_3 & 0xff,

	//IFD end 
	0x00, 0x00, 0x00, 0x00,

	// extra
	0x00, 0x08, 0x00, 0x08, 0x00, 0x08,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0xff, 0x00, 0xff, 0x00, 0xff,
	0x00, 0x01, 0x00, 0x01, 0x00, 0x01
};

void print_screen()
{
	std::string path = "../KinectSLAM/Animation/out-";
	if (printID < 100)
		path += "0";
	if (printID < 10)
		path += "0";
	path += std::to_string(printID);
	path += ".tif";

	char* tmpBuf = (char*)std::malloc(STRIP_SIZE);
	if (tmpBuf == NULL)
	{
		printf("Not enough memory, unable to save screenshot\n");
		return;
	}

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, SCR_WIDTH, SCR_HEIGHT, GL_RGB, GL_UNSIGNED_BYTE, tmpBuf);

	// have to flip upside down since tiff readers ignore orientation

	char* swapMem = (char*)std::malloc(3 * SCR_WIDTH);
	if (swapMem == NULL)
	{
		printf("Not enough memory, unable to save screenshot\n");
		return;
	}

	for (int j = 0; j < SCR_HEIGHT / 2; j++)
	{
		memcpy(swapMem, &tmpBuf[3 * SCR_WIDTH * j], 3 * SCR_WIDTH);
		memcpy(&tmpBuf[3 * SCR_WIDTH * j], &tmpBuf[3 * SCR_WIDTH * (SCR_HEIGHT - j - 1)], 3 * SCR_WIDTH);
		memcpy(&tmpBuf[3 * SCR_WIDTH * (SCR_HEIGHT - j - 1)], swapMem, 3 * SCR_WIDTH);
	}
	std::free(swapMem);

	std::ofstream myFile(path.c_str(), std::ios::out | std::ios::binary);
	myFile.write(TIF_HEADER, 8);
	myFile.write(tmpBuf, STRIP_SIZE);
	myFile.write(TIF_FOOTER, sizeof(TIF_FOOTER));

	if (!myFile)
		printf("Unable to save the current frame to the file: %s\n", path.c_str());
	else
		printf("Saved screenshot to this file: %s\n", path.c_str());

	printID++;

	myFile.close();
	std::free(tmpBuf);
}

#endif