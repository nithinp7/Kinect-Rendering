// Preprocessor Directives
#ifndef KinectSLAM
#define KinectSLAM
#pragma once

#include <glad/glad.h>

//#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <GLFW/glfw3.h>

#include <constants.hpp>

#include <shader.hpp>
#include <shaderResources.hpp>
#include <textures.hpp>

#include <screenQuad.hpp>
#include <skybox.hpp>

#include <camera.hpp>
#include <render.hpp>
#include <tifExport.hpp>
#include <kinect.hpp>

#include <string>
//#include <limits>

#include <math.h>    

#define PI 3.14159265358979323846  /* pi */

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void set_lighting(Shader* shader, glm::vec3* pointLightPositions);

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float framerate = 0.0f;

// booleans for doing different things
bool drawNormals = false;
bool quaterians = false;

// Transformation Matrices
glm::vec3 translation   = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 rotation_rate = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 rotation_euler      = glm::vec3(0.0f, 0.0f, 0.0f);
glm::quat rotation   =   glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
glm::vec3 scale         = glm::vec3(1.0f, 1.0f, 1.0f);

// Step size of transformations
float step_multiplier = 0.1f;

// Last Press
float last_pressed = 0.0f;

// Animate and save each frame
bool animate = false;

int threshold = 1;
bool updateGeom = false;

#endif 
