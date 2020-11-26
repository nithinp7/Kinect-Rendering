#ifndef SCREEN_QUAD_H
#define SCREEN_QUAD_H

#include <glad/glad.h>

#include <shader.hpp>

#include <vector>

namespace ScreenQuad {

	const float verts[] = {
		// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,

		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};

	void init();
	void draw(Shader* shader, GLuint texture);
	void destroy();

};

#endif