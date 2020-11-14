#ifndef TEXTURES_H
#define TEXTURES_H

#include <glad/glad.h>
#include <iostream>
#include <vector>
#include <string>

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const* path);

// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -------------------------------------------------------
unsigned int loadCubemap(std::vector<std::string> faces);

#endif