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


struct Boid {
	glm::vec3 position;
	glm::vec3 velocity;

	Boid(glm::vec3 pos, glm::vec3 vel) : position(pos), velocity(vel) {}
};


const float MAX_SPEED = 0.05f;
const float MAX_FORCE = 0.02f;
const float NEIGHBOR_RADIUS = 1.5f;
const float SEPARATION_RADIUS = 0.5f;

std::vector<Boid> boids;

const float BOUNDARY_X = 5.0f;
const float BOUNDARY_Y = 5.0f;
const float BOUNDARY_Z = 5.0f;
const float TURN_FORCE = 0.05f;


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
Core::RenderContext sphereContext;

glm::vec3 cameraPos = glm::vec3(-4.f, 0, 0);
glm::vec3 cameraDir = glm::vec3(1.f, 0.f, 0.f);

glm::vec3 spaceshipPos = glm::vec3(-4.f, 0, 0);
glm::vec3 spaceshipDir = glm::vec3(1.f, 0.f, 0.f);
GLuint VAO, VBO;

float aspectRatio = 1.f;


glm::vec3 checkBoundaries(Boid& boid) {
	glm::vec3 steer(0.0f);

	if (boid.position.x > BOUNDARY_X) steer.x = -TURN_FORCE;
	else if (boid.position.x < -BOUNDARY_X) steer.x = TURN_FORCE;

	if (boid.position.y > BOUNDARY_Y) steer.y = -TURN_FORCE;
	else if (boid.position.y < -BOUNDARY_Y) steer.y = TURN_FORCE;

	if (boid.position.z > BOUNDARY_Z) steer.z = -TURN_FORCE;
	else if (boid.position.z < -BOUNDARY_Z) steer.z = TURN_FORCE;

	return steer;
}


glm::vec3 separation(Boid& boid) {
	glm::vec3 steer(0.0f);
	int count = 0;
	for (auto& other : boids) {
		float distance = glm::length(boid.position - other.position);
		if (distance > 0 && distance < SEPARATION_RADIUS) {
			glm::vec3 diff = glm::normalize(boid.position - other.position) / distance;
			steer += diff;
			count++;
		}
	}
	if (count > 0) {
		steer /= (float)count;
	}
	if (glm::length(steer) > 0) {
		steer = glm::normalize(steer) * MAX_SPEED - boid.velocity;
		steer = glm::clamp(steer, -MAX_FORCE, MAX_FORCE);
	}
	return steer;
}


glm::vec3 alignment(Boid& boid) {
	glm::vec3 avgVelocity(0.0f);
	int count = 0;
	for (auto& other : boids) {
		float distance = glm::length(boid.position - other.position);
		if (distance > 0 && distance < NEIGHBOR_RADIUS) {
			avgVelocity += other.velocity;
			count++;
		}
	}
	if (count > 0) {
		avgVelocity /= (float)count;
		avgVelocity = glm::normalize(avgVelocity) * MAX_SPEED;
		glm::vec3 steer = avgVelocity - boid.velocity;
		return glm::clamp(steer, -MAX_FORCE, MAX_FORCE);
	}
	return glm::vec3(0.0f);
}


glm::vec3 cohesion(Boid& boid) {
	glm::vec3 center(0.0f);
	int count = 0;
	for (auto& other : boids) {
		float distance = glm::length(boid.position - other.position);
		if (distance > 0 && distance < NEIGHBOR_RADIUS) {
			center += other.position;
			count++;
		}
	}
	if (count > 0) {
		center /= (float)count;
		glm::vec3 steer = glm::normalize(center - boid.position) * MAX_SPEED - boid.velocity;
		return glm::clamp(steer, -MAX_FORCE, MAX_FORCE);
	}
	return glm::vec3(0.0f);
}


void updateBoids() {
	for (auto& boid : boids) {
		glm::vec3 sep = separation(boid);
		glm::vec3 align = alignment(boid);
		glm::vec3 coh = cohesion(boid);
		glm::vec3 boundaryAvoidance = checkBoundaries(boid);

		boid.velocity += sep + align + coh + boundaryAvoidance;
		boid.velocity = glm::normalize(boid.velocity) * MAX_SPEED;
		boid.position += boid.velocity;
	}
}


void drawBoundingBox() {
	glUseProgram(program);
	glm::mat4 modelMatrix = glm::scale(glm::vec3(BOUNDARY_X, BOUNDARY_Y, BOUNDARY_Z));

	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, &modelMatrix[0][0]);

	glBegin(GL_LINES);
	glColor3f(1, 1, 1);

	for (int i = -1; i <= 1; i += 2) {
		for (int j = -1; j <= 1; j += 2) {
			for (int k = -1; k <= 1; k += 2) {
				glVertex3f(i * BOUNDARY_X, j * BOUNDARY_Y, -BOUNDARY_Z);
				glVertex3f(i * BOUNDARY_X, j * BOUNDARY_Y, BOUNDARY_Z);

				glVertex3f(i * BOUNDARY_X, -BOUNDARY_Y, k * BOUNDARY_Z);
				glVertex3f(i * BOUNDARY_X, BOUNDARY_Y, k * BOUNDARY_Z);

				glVertex3f(-BOUNDARY_X, i * BOUNDARY_Y, k * BOUNDARY_Z);
				glVertex3f(BOUNDARY_X, i * BOUNDARY_Y, k * BOUNDARY_Z);
			}
		}
	}

	glEnd();
	glUseProgram(0);
}


void initBoids(int numBoids) {
	for (int i = 0; i < numBoids; i++) {
		glm::vec3 pos = glm::vec3(
			((rand() % 100) / 50.0f) - 1,
			((rand() % 100) / 50.0f) - 1,
			((rand() % 100) / 50.0f) - 1
		);

		glm::vec3 vel = glm::normalize(glm::vec3(
			((rand() % 100) / 50.0f) - 1,
			((rand() % 100) / 50.0f) - 1,
			((rand() % 100) / 50.0f) - 1
		)) * MAX_SPEED;

		boids.emplace_back(pos, vel);
	}
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
	float f = 20.;
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


void renderBoids() {
	for (auto& boid : boids) {
		glm::mat4 modelMatrix = glm::translate(boid.position) * glm::scale(glm::vec3(0.05f));
		drawObjectTexture(sphereContext, modelMatrix, texture::ship);
	}
}


void renderScene(GLFWwindow* window)
{
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 transformation;
	float time = glfwGetTime();

	renderBoids();
	drawBoundingBox();

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
	const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_CalcTangentSpace);

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

	loadModelToContext("./models/sphere.objj", sphereContext);

	initBoids(100);
}

void shutdown(GLFWwindow* window)
{
	shaderLoader.DeleteProgram(program);
}


void processInput(GLFWwindow* window)
{
	glm::vec3 spaceshipSide = glm::normalize(glm::cross(spaceshipDir, glm::vec3(0.f, 1.f, 0.f)));
	glm::vec3 spaceshipUp = glm::vec3(0.f, 1.f, 0.f);
	float angleSpeed = 0.05f;
	float moveSpeed = 0.025f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		spaceshipPos += spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		spaceshipPos -= spaceshipDir * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
		spaceshipPos += spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
		spaceshipPos -= spaceshipSide * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		spaceshipPos += spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		spaceshipPos -= spaceshipUp * moveSpeed;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(angleSpeed) * glm::vec4(spaceshipDir, 0));
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		spaceshipDir = glm::vec3(glm::eulerAngleY(-angleSpeed) * glm::vec4(spaceshipDir, 0));

	cameraPos = spaceshipPos - 1.5 * spaceshipDir + glm::vec3(0, 1, 0) * 0.5f;
	cameraDir = spaceshipDir;

	//cameraDir = glm::normalize(-cameraPos);

}

void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window))
	{
		processInput(window);
		updateBoids();
		renderScene(window);
		glfwPollEvents();
	}
}
