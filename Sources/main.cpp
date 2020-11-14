
#include <main.hpp>

std::string preamble = 
	"Kinect SLAM\n\n"
	"Press P to save a screenshot\n\n";

int main()
{
	// glfw: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

	std:printf(preamble.c_str());
														 // glfw window creation
														 // --------------------
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Kinect SLAM", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// configure global opengl state
	// -----------------------------
	glEnable(GL_MULTISAMPLE); // Enabled by default on some drivers, but not all so always enable to make sure
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	shaders = ShaderResources::get_instance();

	Skybox* skybox = new Skybox();
	initScreenQuad();

	// positions of the point lights
	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	};

	//Render render = Render();
	unsigned int box_texture = loadTexture("../KinectSLAM/Media/textures/container.jpg");
	unsigned int smile_texture = loadTexture("../KinectSLAM/Media/textures/awesomeface.png");
	unsigned int grey_texture = loadTexture("../KinectSLAM/Media/textures/grey.png");

	MarchingCubes* mcubes = new MarchingCubes();
	Kinect* kinect = new Kinect();

	// kinect related
	if (kinect->init_error_flag)
	{
		printf("Failed to init Kinect!\n");
		exit(-1);
	}

	// render loop
	// -----------
	while (!glfwWindowShouldClose(window))
	{
		// per-frame time logic
		// --------------------
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// weighted avg for framerate
		framerate = (0.4f / (deltaTime)+1.6f * framerate) / 2.0f;

		// input
		// -----
		processInput(window);

		// render
		// ------
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
		
		shaders->update(view, projection);

		set_lighting(shaders->renderPass, pointLightPositions);

		if(!freezeKinect) kinect->update();
		kinect->draw();

		//float sc = 0.45f;
		// 3.6 meters between the near and far plane 
		float sc = 3.6f;

		glm::mat4 model;
		//model = glm::translate(model, glm::vec3(-10.0f, 5.0f, -50.0f));
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, -2.2f));
		model = glm::scale(model, glm::vec3(sc, sc, sc));

		//mcubes->Draw(model, threshold, updateGeom);
		updateGeom = false;

		/*
		if (drawNormals) 
			//blahblah Draw(normalShader);
		*/

		// draw skybox as last
		skybox->draw();
		
		// save frame to file if print flag is set
		if (print)
		{
			print_screen();
			print = false;
		}

		// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		// -------------------------------------------------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// release resources  
	delete mcubes;
	delete kinect;
	delete skybox;
	deleteScreenQuad();

	ShaderResources::reset_instance();
	shaders = NULL;

	glfwTerminate();
	return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
	// Escape Key quits
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Movement Keys
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);

	static float offsX = 0.014f, offsY = -0.004f;

	// debounced button presses
	float currentFrame = glfwGetTime();
	bool somethingPressed = glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS ||
							glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS ||
							glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS ||
							glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS ||
							glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS ||
							glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
	if (somethingPressed && last_pressed < currentFrame - 0.5f || last_pressed == 0.0f)
	{
		if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
			//threshold = 1;
			//updateGeom = true;
			offsY -= 0.001;
			printf("offsY set to %f meters\n", offsY);
		}
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			//threshold += 20;//threshold = 90;
			//updateGeom = true;
			offsX += 0.001;
			printf("offsX set to %f meters\n", offsX);
		}
		if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS) {
			threshold = 400;
			updateGeom = true;
		}
		if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
			freezeKinect = !freezeKinect;
		}
		// Print screenshot
		if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
		{
			print = true;
		}


		last_pressed = currentFrame;
	}

	// TODO: remove this (FOR DEBUG ONLY)
	shaders->kinectPointCloud->use();
	shaders->kinectPointCloud->setFloat("offsX", offsX);
	shaders->kinectPointCloud->setFloat("offsY", offsY);
	
	// modifiers if needed
	// bool shift = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT);
	// bool ctrl = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) || glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	//glViewport(0, 0, width, height);
	//SCR_WIDTH = width;
	//SCR_HEIGHT = height;

}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

void set_lighting(Shader* shader, glm::vec3* pointLightPositions)
{
	shader->use();
	shader->setVec3("viewPos", camera.Position);

	// directional light
	//shader->setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	shader->setVec3("dirLight.direction", 0.24f, -.3f, -0.91f); // Tried to target the sun
	shader->setVec3("dirLight.ambient", 0.3f, 0.3f, 0.3f);
	//shader->setVec3("dirLight.ambient", 0.0f, 0.0f, 0.0f);
	//shader->setVec3("dirLight.diffuse", 0.0f, 0.0f, 0.0f);
	//shader->setVec3("dirLight.diffuse", 0.6f, 0.6f, 0.6f);
	shader->setVec3("dirLight.diffuse", 0.3f, 0.3f, 0.6f);
	shader->setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);

	// point light 1
	shader->setVec3("pointLights[0].position", pointLightPositions[0]);
	shader->setVec3("pointLights[0].ambient", 0.05f, 0.05f, 0.05f);
	shader->setVec3("pointLights[0].diffuse", 0.8f, 0.8f, 0.8f);
	shader->setVec3("pointLights[0].specular", 1.0f, 1.0f, 1.0f);
	shader->setFloat("pointLights[0].constant", 1.0f);
	shader->setFloat("pointLights[0].linear", 0.09);
	shader->setFloat("pointLights[0].quadratic", 0.032);
	// point light 2
	shader->setVec3("pointLights[1].position", pointLightPositions[1]);
	shader->setVec3("pointLights[1].ambient", 0.05f, 0.05f, 0.05f);
	shader->setVec3("pointLights[1].diffuse", 0.8f, 0.8f, 0.8f);
	shader->setVec3("pointLights[1].specular", 1.0f, 1.0f, 1.0f);
	shader->setFloat("pointLights[1].constant", 1.0f);
	shader->setFloat("pointLights[1].linear", 0.09);
	shader->setFloat("pointLights[1].quadratic", 0.032);
	// point light 3
	shader->setVec3("pointLights[2].position", pointLightPositions[2]);
	shader->setVec3("pointLights[2].ambient", 0.05f, 0.05f, 0.05f);
	shader->setVec3("pointLights[2].diffuse", 0.8f, 0.8f, 0.8f);
	shader->setVec3("pointLights[2].specular", 1.0f, 1.0f, 1.0f);
	shader->setFloat("pointLights[2].constant", 1.0f);
	shader->setFloat("pointLights[2].linear", 0.09);
	shader->setFloat("pointLights[2].quadratic", 0.032);
	// point light 4
	shader->setVec3("pointLights[3].position", pointLightPositions[3]);
	shader->setVec3("pointLights[3].ambient", 0.05f, 0.05f, 0.05f);
	shader->setVec3("pointLights[3].diffuse", 0.8f, 0.8f, 0.8f);
	shader->setVec3("pointLights[3].specular", 1.0f, 1.0f, 1.0f);
	shader->setFloat("pointLights[3].constant", 1.0f);
	shader->setFloat("pointLights[3].linear", 0.09);
	shader->setFloat("pointLights[3].quadratic", 0.032);
	// spotLight
	shader->setVec3("spotLight.position", camera.Position);
	shader->setVec3("spotLight.direction", camera.Front);
	shader->setVec3("spotLight.ambient", 0.0f, 0.0f, 0.0f);
	shader->setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	shader->setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
	shader->setFloat("spotLight.constant", 0.0f);
	shader->setFloat("spotLight.linear", 0.001f);
	shader->setFloat("spotLight.quadratic", 0.0009f);
	shader->setFloat("spotLight.cutOff", glm::cos(glm::radians(17.5f)));
	shader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(22.0f)));

}