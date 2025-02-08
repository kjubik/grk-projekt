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
#include "utils.h"

SimulationParams simulationParams;

class Boid {
public:
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 scale;
	Core::RenderContext modelContext;

	static constexpr float MAX_SPEED = 2.0f;
	static constexpr float MIN_SPEED = 0.2f;

	Boid(glm::vec3 pos, glm::vec3 vel, const Core::RenderContext& context, glm::vec3 scl = glm::vec3(1.0f))
		: position(pos), velocity(vel), modelContext(context), scale(scl) {
	}

	void update(float deltaTime, const glm::vec3& acceleration) {
		position += velocity * deltaTime;
		velocity += acceleration * deltaTime;

		applyBounceForceFromBoundingBox(deltaTime);
		limitSpeed();
	}

	void draw(GLuint shaderProgram, GLuint modelLoc, const glm::mat4& view,
		const glm::mat4& projection, GLuint viewLoc, GLuint projectionLoc) {
		glUseProgram(shaderProgram);

		GLint inputColorLocation = glGetUniformLocation(shaderProgram, "inputColor");
		glUniform3f(inputColorLocation, 0.7f, 0.7f, 0.7f);

		glm::mat4 rotationMatrix = getRotationMatrixFromVelocity(velocity);

		glm::mat4 model = glm::translate(glm::mat4(1.0f), position) * rotationMatrix * glm::scale(glm::mat4(1.0f), scale);

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

		Core::DrawContext(modelContext);

		glUseProgram(0);
	}

private:
	glm::mat4 getRotationMatrixFromVelocity(const glm::vec3& velocity) {
		glm::vec3 forward = glm::vec3(0.0f, -1.0f, 0.0f);

		glm::vec3 normalizedVelocity = glm::normalize(velocity);
		if (glm::length(normalizedVelocity) < 1e-6f) {
			throw std::invalid_argument("Velocity vector cannot be zero.");
		}

		if (glm::length(normalizedVelocity + forward) < 1e-6f) {
			return glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
		}

		if (glm::length(glm::cross(forward, normalizedVelocity)) < 1e-6f) {
			return glm::mat4(1.0f);
		}

		glm::vec3 rotationAxis = glm::normalize(glm::cross(forward, normalizedVelocity));

		float cosTheta = glm::dot(forward, normalizedVelocity);
		float theta = std::acos(glm::clamp(cosTheta, -1.0f, 1.0f));

		glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), theta, rotationAxis);

		return rotationMatrix;
	}
	void applyBounceForceFromBoundingBox(float deltaTime) {
		glm::vec3 bounceForce(0.0f);

		if (position.x < simulationParams.boundMin) {
			bounceForce.x += simulationParams.bounceForce * (simulationParams.boundMin - position.x);
		}
		else if (position.x > simulationParams.boundMax) {
			bounceForce.x -= simulationParams.bounceForce * (position.x - simulationParams.boundMax);
		}

		if (position.y < simulationParams.boundMin) {
			bounceForce.y += simulationParams.bounceForce * (simulationParams.boundMin - position.y);
		}
		else if (position.y > simulationParams.boundMax) {
			bounceForce.y -= simulationParams.bounceForce * (position.y - simulationParams.boundMax);
		}

		if (position.z < simulationParams.boundMin) {
			bounceForce.z += simulationParams.bounceForce * (simulationParams.boundMin - position.z);
		}
		else if (position.z > simulationParams.boundMax) {
			bounceForce.z -= simulationParams.bounceForce * (position.z - simulationParams.boundMax);
		}

		velocity += bounceForce * deltaTime;
	}

	void limitSpeed() {
		float speed = glm::length(velocity);
		if (speed > MAX_SPEED) {
			velocity = glm::normalize(velocity) * MAX_SPEED;
		}
		else if (speed < MIN_SPEED) {
			velocity = glm::normalize(velocity) * MIN_SPEED;
		}
	}
	};

class Flock {
public:
	std::vector<Boid> boids;
	Flock() {}

	Flock(int numBoids, const Core::RenderContext& modelContext) {
		for (int i = 0; i < numBoids; ++i) {
			glm::vec3 position = glm::vec3(
				static_cast<float>(std::rand()) / RAND_MAX * 4.0f - 2.0f,
				static_cast<float>(std::rand()) / RAND_MAX * 4.0f - 2.0f,
				static_cast<float>(std::rand()) / RAND_MAX * 4.0f - 2.0f
			);

			glm::vec3 velocity = glm::normalize(glm::vec3(
				static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f,
				static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f,
				static_cast<float>(std::rand()) / RAND_MAX * 2.0f - 1.0f
			)) * (static_cast<float>(std::rand()) / RAND_MAX * 2.0f);

			boids.emplace_back(position, velocity, modelContext, glm::vec3(simulationParams.boidModelScale));
		}
	}

	void update(float deltaTime) {
		for (auto& boid : boids) {
			glm::vec3 avoidance = computeAvoidance(boid);
			glm::vec3 alignment = computeAlignment(boid);
			glm::vec3 cohesion = computeCohesion(boid);
			boid.update(deltaTime, avoidance + alignment + cohesion);
		}
	}

	void draw(GLuint shaderProgram, GLuint modelLoc, const glm::mat4& view, const glm::mat4& projection, GLuint viewLoc, GLuint projectionLoc) {
		for (auto& boid : boids) {
			boid.draw(shaderProgram, modelLoc, view, projection, viewLoc, projectionLoc);
		}
	}
private:
	glm::vec3 computeAvoidance(const Boid& boid) {
		glm::vec3 avoidance(0.0f);
		int count = 0;

		for (const auto& other : boids) {
			if (&other == &boid) continue;

			float distance = glm::length(boid.position - other.position);
			if (distance < simulationParams.avoidRadius && distance > 0.0f) {
				avoidance += glm::normalize(boid.position - other.position) / (distance * distance);
				count++;
			}
		}

		if (count > 0) {
			avoidance /= static_cast<float>(count);
			avoidance *= simulationParams.avoidForce;
		}
		return avoidance;
	}

	glm::vec3 computeAlignment(const Boid& boid) {
		glm::vec3 averageVelocity(0.0f);
		int count = 0;

		for (const auto& other : boids) {
			if (&other == &boid) continue;

			float distance = glm::length(boid.position - other.position);
			if (distance < simulationParams.alignRadius) {
				averageVelocity += other.velocity;
				count++;
			}
		}

		if (count > 0) {
			averageVelocity /= static_cast<float>(count);
			averageVelocity = glm::normalize(averageVelocity) * simulationParams.alignForce;
		}
		return averageVelocity;
	}

	glm::vec3 computeCohesion(const Boid& boid) {
		glm::vec3 centerOfMass(0.0f);
		int count = 0;

		for (const auto& other : boids) {
			if (&other == &boid) continue;

			float distance = glm::length(boid.position - other.position);
			if (distance < simulationParams.cohesionRadius) {
				centerOfMass += other.position;
				count++;
			}
		}

		if (count > 0) {
			centerOfMass /= static_cast<float>(count);
			glm::vec3 cohesionForce = glm::normalize(centerOfMass - boid.position) * simulationParams.cohesionForce;
			return cohesionForce;
		}

		return glm::vec3(0.0f);
	}
};

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

glm::vec3 spaceshipPos = glm::vec3(-15.f, 0, 0);
glm::vec3 spaceshipDir = glm::vec3(1.f, 0.f, 0.f);
glm::vec3 cameraPos = glm::vec3(0, 0, 1.0f);
glm::vec3 cameraDir = glm::vec3(1.0f, 0.0f, 0.0f);

GLuint VAO, VBO;

float aspectRatio = 1.f;

GLuint boidVAO, boidVBO;
GLuint boundingBoxVAO, boundingBoxVBO, boundingBoxEBO;

GLuint boidShader, boundBoxShader;
Flock flock;

GLuint modelLoc;
GLuint viewLoc;
GLuint projectionLoc;

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
	float f = 50.;
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

void renderScene(GLFWwindow* window)
{
	glClearColor(0.1f, 0.3f, 0.6f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glm::mat4 transformation;
	float time = glfwGetTime();

	glm::mat4 projection = createPerspectiveMatrix();
	glm::mat4 view = createCameraMatrix();

	drawBoundingBox(view, projection, boundBoxShader, boundingBoxVAO);
	flock.draw(boidShader, modelLoc, view, projection, viewLoc, projectionLoc);

	glm::mat4 treeModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(2.5f));
	treeModelMatrix = glm::translate(treeModelMatrix, glm::vec3(0.0f, -4.0f, 0.0f));
	drawObjectColor(treeContext, treeModelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));

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

	loadModelToContext("./models/bird.objj", birdContext);
	loadModelToContext("./models/tree.objj", treeContext);

	setupBoidVAOandVBO(boidVAO, boidVBO, boidVertices, sizeof(boidVertices));
	setupBoundingBox(boundingBoxVAO, boundingBoxVBO, boundingBoxEBO);

	boidShader = shaderLoader.CreateProgram("shaders/boid.vert", "shaders/boid.frag");
	boundBoxShader = shaderLoader.CreateProgram("shaders/line.vert", "shaders/line.frag");

	modelLoc = glGetUniformLocation(boidShader, "model");
	viewLoc = glGetUniformLocation(boidShader, "view");
	projectionLoc = glGetUniformLocation(boidShader, "projection");

	initWidget(window);

	flock = Flock(simulationParams.boidNumber, birdContext);
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
		renderScene(window);
		flock.update(simulationParams.deltaTime);
		glfwPollEvents();
	}
	destroyWidget();
}
