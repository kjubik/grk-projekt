#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Texture.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

#include <vector>

#include "boids/simulation.h"
#include "boids/vertices.h"
#include "boids/Terrain.h"
#include "boids/Boid.h"
#include "utils.h"

#include <random>
#include <numeric>

#include "SOIL/SOIL.h"

bool wireframeOnlyView = false;
bool key1WasPressed = false;
bool key2WasPressed = false;
bool cursorEnabled = false;

SimulationParams simulationParams;

glm::vec3 cameraPos = glm::vec3(-15.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);
GLuint boidTextureID;
GLuint gradientTextures[10];

float yaw = 0.0f;
float pitch = 0.0f;
float lastX = 600.0f / 2.0f;
float lastY = 600.0f / 2.0f;
bool firstMouse = true;
bool escapeWasPressed = false;

const float mouseSensitivity = 0.1f;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (cursorEnabled) {
		return;
	}

	if (firstMouse) {
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xOffset = xpos - lastX;
	float yOffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	xOffset *= mouseSensitivity;
	yOffset *= mouseSensitivity;

	yaw += xOffset;
	pitch += yOffset;

	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	cameraDir = glm::normalize(direction);
}

namespace texture {
	GLuint earth;
	GLuint clouds;
	GLuint moon;
	GLuint ship;

	GLuint grid;

	GLuint earthNormal;
	GLuint asteroidNormal;
	GLuint shipNormal;
}

GLuint program;
GLuint programSun;
GLuint programTex;
GLuint programEarth;
GLuint programProcTex;
Core::Shader_Loader shaderLoader;

Core::RenderContext shipContext;
Core::RenderContext birdContext;
Core::RenderContext treeContext;

GLuint VAO, VBO;

float aspectRatio = 1.f;

ProceduralTerrain* terrain;
GLuint boidVAO, boidVBO;
GLuint boundingBoxVAO, boundingBoxVBO, boundingBoxEBO;

GLuint boidShader, basicBoidShader, boundBoxShader, terrainShader;
GLuint activeBoidShader; 

Flock flock;

GLuint modelLoc;
GLuint viewLoc;
GLuint projectionLoc;

GLuint terrainProjectionLoc, terrainViewLoc, terrainModelLoc, terrainColorLoc;
GLuint terrainTexture, terrainNormal;

GLuint skyboxTexture;
GLuint skyboxShader;
Core::RenderContext skyboxCube;


std::vector<std::string> skyboxFaces = {
	"./textures/skybox/clouds/clouds1_east.bmp",
	"./textures/skybox/clouds/clouds1_west.bmp",
	"./textures/skybox/clouds/clouds1_up.bmp",
	"./textures/skybox/clouds/clouds1_down.bmp",
	"./textures/skybox/clouds/clouds1_north.bmp",
	"./textures/skybox/clouds/clouds1_south.bmp",
};


GLuint loadCubemap(const std::vector<std::string>& faces) {
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (size_t i = 0; i < faces.size(); i++) {
		unsigned char* data = SOIL_load_image(faces[i].c_str(), &width, &height, &nrChannels, SOIL_LOAD_RGBA);
		if (data) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			std::cout << "Loaded: " << faces[i] << std::endl;
		}
		else {
			std::cerr << "Failed to load: " << faces[i] << std::endl;
		}
		SOIL_free_image_data(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}


glm::mat4 createCameraMatrix()
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::normalize(glm::cross(cameraSide, cameraDir));
	glm::mat4 cameraRotrationMatrix = glm::mat4({
		cameraSide.x,cameraSide.y,cameraSide.z,0,
		cameraUp.x,cameraUp.y,cameraUp.z ,0,
		-cameraDir.x,-cameraDir.y,-cameraDir.z,0,
		0.,0.,0.,1.,
		});
	cameraRotrationMatrix = glm::transpose(cameraRotrationMatrix);
	glm::mat4 cameraMatrix = cameraRotrationMatrix * glm::translate(-cameraPos);

	return cameraMatrix;
}

glm::mat4 createPerspectiveMatrix()
{
	glm::mat4 perspectiveMatrix;
	float n = 0.05;
	float f = 1000.;
	float a1 = glm::min(aspectRatio, 1.f);
	float a2 = glm::min(1 / aspectRatio, 1.f);
	perspectiveMatrix = glm::mat4({
		1,0.,0.,0.,
		0.,aspectRatio,0.,0.,
		0.,0.,(f + n) / (n - f),2 * f * n / (n - f),
		0.,0.,-1.,0.,
		});

	perspectiveMatrix = glm::transpose(perspectiveMatrix);

	return perspectiveMatrix;
}

void drawObjectColor(Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {

	glUseProgram(program);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(program, "lightPos"), 0, 0, 0);
	Core::DrawContext(context);

}

void drawObjectTexture(Core::RenderContext& context, glm::mat4 modelMatrix, GLuint textureID) {
	glUseProgram(programTex);
	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(programTex, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(programTex, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(programTex, "lightPos"), 0, 0, 0);
	Core::SetActiveTexture(textureID, "colorTexture", programTex, 0);
	Core::DrawContext(context);
}


void drawSkybox() {
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glUseProgram(skyboxShader);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * glm::mat4(glm::mat3(createCameraMatrix()));
	glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "viewProjection"), 1, GL_FALSE, &viewProjectionMatrix[0][0]);

	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTexture);
	Core::DrawContext(skyboxCube);

	glDepthMask(GL_TRUE);
	glUseProgram(0);
}


void renderScene(GLFWwindow* window)
{
	glClearColor(0.1f, 0.3f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 transformation;
	float time = glfwGetTime();

	drawSkybox();

	glm::mat4 projection = createPerspectiveMatrix();
	glm::mat4 view = createCameraMatrix();

	drawBoundingBox(view, projection, boundBoxShader, boundingBoxVAO);
	flock.draw(activeBoidShader, modelLoc, view, projection, viewLoc, projectionLoc, cameraPos);

	if (terrain)
		terrain->render(terrainShader, projection, view, glm::mat4(1.0f), terrainTexture, terrainNormal, cameraPos, lightPos);

	//glm::mat4 treeModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.5f));
	//treeModelMatrix = glm::translate(treeModelMatrix, glm::vec3(0.0f, -4.0f, 0.0f));
	//drawObjectColor(treeContext, treeModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));

	drawSliderWidget(&simulationParams);

	glUseProgram(0);
	glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
}

void loadModelToContext(std::string path, Core::RenderContext& context)
{
	Assimp::Importer import;
	const aiScene* scene = import.ReadFile(path,
		aiProcess_Triangulate |
		aiProcess_CalcTangentSpace |
		aiProcess_JoinIdenticalVertices |
		aiProcess_GenSmoothNormals |
		aiProcess_ImproveCacheLocality |
		aiProcess_RemoveRedundantMaterials |
		aiProcess_OptimizeMeshes);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
		return;
	}
	context.initFromAssimpMesh(scene->mMeshes[0]);
}

void init(GLFWwindow* window)
{
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glEnable(GL_DEPTH_TEST);
	program = shaderLoader.CreateProgram("shaders/shader_5_1.vert", "shaders/shader_5_1.frag");
	programTex = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");
	programEarth = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");
	programProcTex = shaderLoader.CreateProgram("shaders/shader_5_1_tex.vert", "shaders/shader_5_1_tex.frag");

	loadModelToContext("./models/bird.objj", birdContext);
	loadModelToContext("./models/tree.objj", treeContext);

	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	for (int i = 0; i < 10; ++i) {
		std::string texturePath = "textures/gradient_" + std::to_string(i+1) + ".png";
		gradientTextures[i] = Core::LoadTexture(texturePath.c_str());
	}

	terrainTexture = Core::LoadTexture("textures/terrain/rocky.jpg");
	terrainNormal = Core::LoadTexture("textures/terrain/normal.jpg");

	setupBoidVAOandVBO(boidVAO, boidVBO, boidVertices, sizeof(boidVertices));
	setupBoundingBox(boundingBoxVAO, boundingBoxVBO, boundingBoxEBO);

	terrain = new ProceduralTerrain(150.0f, 50);
	terrain->translateTerrain(glm::vec3(0.0f, -14.0f, 0.0f));

	boidShader = shaderLoader.CreateProgram("shaders/boid.vert", "shaders/boid.frag");
	basicBoidShader = shaderLoader.CreateProgram("shaders/boid_basic.vert", "shaders/boid_basic.frag");
	boundBoxShader = shaderLoader.CreateProgram("shaders/line.vert", "shaders/line.frag");
	terrainShader = shaderLoader.CreateProgram("shaders/terrain.vert", "shaders/terrain.frag");

	activeBoidShader = boidShader;

	modelLoc = glGetUniformLocation(boidShader, "model");
	viewLoc = glGetUniformLocation(boidShader, "view");
	projectionLoc = glGetUniformLocation(boidShader, "projection");

	terrainProjectionLoc = glGetUniformLocation(terrainShader, "projection");
	terrainViewLoc = glGetUniformLocation(terrainShader, "view");
	terrainModelLoc = glGetUniformLocation(terrainShader, "model");
	terrainColorLoc = glGetUniformLocation(terrainShader, "objectColor");
  
	initWidget(window);

	flock = Flock(&simulationParams, terrain, birdContext, gradientTextures);

	skyboxShader = shaderLoader.CreateProgram("shaders/skybox.vert", "shaders/skybox.frag");
	skyboxTexture = loadCubemap(skyboxFaces);
	loadModelToContext("./models/cube.objj", skyboxCube);
}

void shutdown(GLFWwindow* window)
{
	shaderLoader.DeleteProgram(program);
	if (terrain) {
		delete terrain;
		terrain = nullptr;
	}
}

void processInput(GLFWwindow* window)
{
	glm::vec3 cameraSide = glm::normalize(glm::cross(cameraDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 cameraUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.075f;
	float moveSpeed = 0.1f;

	/*if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);*/
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += cameraDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= cameraDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= cameraSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += cameraSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		cameraPos += cameraUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		cameraPos -= cameraUp * moveSpeed;

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
		key1WasPressed = true;
	}
	else {
		if (key1WasPressed) {
			terrain->wireframeOnlyView = !terrain->wireframeOnlyView;
			key1WasPressed = false;
		}
	}

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
		if (!key2WasPressed) {
			activeBoidShader = (activeBoidShader == boidShader) ? basicBoidShader : boidShader;

			modelLoc = glGetUniformLocation(activeBoidShader, "model");
			viewLoc = glGetUniformLocation(activeBoidShader, "view");
			projectionLoc = glGetUniformLocation(activeBoidShader, "projection");

			key2WasPressed = true;
		}
	}
	else {
		key2WasPressed = false;
	}

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		if (!escapeWasPressed) {
			cursorEnabled = !cursorEnabled;
			glfwSetInputMode(window, GLFW_CURSOR,
				cursorEnabled ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);

			if (!cursorEnabled) {
				firstMouse = true;
			}
			escapeWasPressed = true;
		}
	}
	else {
		escapeWasPressed = false;
	}
}

void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		renderScene(window);
		flock.update(simulationParams.deltaTime);
		glfwPollEvents();
	}
	destroyWidget();
}
