#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include "Shader_Loader.h"
#include "Render_Utils.h"
//#include "Texture.h"

#include "Box.cpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

Core::Shader_Loader shaderLoader;
Core::RenderContext boxContext;

glm::vec3 cameraPos = glm::vec3(2.0f, 2.0f, 2.0f);
glm::vec3 cameraDir = glm::normalize(glm::vec3(-1.f, -1.f, -1.f));

GLuint program;
GLuint VAO, VBO;

float aspectRatio = 1.f;
const float fov = glm::radians(90.0f);

glm::mat4 createCameraMatrix() {
	return glm::lookAt(cameraPos, cameraPos + cameraDir, glm::vec3(0.f, 1.f, 0.f));
}

glm::mat4 createPerspectiveMatrix() {
	float nearPlane = 0.05f;
	float farPlane = 20.0f;
	return glm::perspective(fov, aspectRatio, nearPlane, farPlane);
}

void drawObjectColor(GLuint program, Core::RenderContext& context, glm::mat4 modelMatrix, glm::vec3 color) {
	glUseProgram(program);

	glm::mat4 viewProjectionMatrix = createPerspectiveMatrix() * createCameraMatrix();
	glm::mat4 transformation = viewProjectionMatrix * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);
	glUniform3f(glGetUniformLocation(program, "color"), color.x, color.y, color.z);
	glUniform3f(glGetUniformLocation(program, "lightDir"), -1.0f, -1.0f, 1.0f); // Kierunek światła
	glUniform3f(glGetUniformLocation(program, "lightColor"), 1.0f, 1.0f, 1.0f); // Białe światło
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glUseProgram(0);
}

float baseWidth = 1.0f;
float baseHeight = 0.25f;

float arm1Width = 0.25f;
float arm1Length = 0.25f;
float arm1Angle = 0.0f;

float pipeWidth = 0.1f;
float pipeHeight = 1.5f;

float arm2Width = 0.25f;
float arm2Length = 1.0f;
float arm2Heigh = pipeHeight * 0.5f;

float arm3Width = 0.24f;
float arm3Length = 0.5f;
float arm3Angle = 90.0f;

float arm4Width = 0.24f;
float arm4Length = 0.25f;
float arm4Angle = 45.0f;

float arm5Width = 0.1f;
float arm5Length = 0.5f;
float arm5Angle = 90.0f;

void renderScene(GLFWwindow* window) {
	glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(program);

	// Ziemia - podłoże
	glm::mat4 groundModel = glm::scale(glm::vec3(10.0, 0.01, 10.0)); // Skalowanie, aby utworzyć szeroką, płaską powierzchnię
	drawObjectColor(program, boxContext, groundModel, glm::vec3(0.75f)); // Rysowanie podłoża w szarym kolorze

	// Podstawa Robota
	glm::mat4 baseTranslate = glm::translate(glm::vec3(0.0)); // Translacja (na razie brak przesunięcia)
	glm::mat4 baseModel = baseTranslate; // Inicjalizacja macierzy modelu podstawy
	baseModel = glm::scale(baseModel, glm::vec3(baseWidth, baseHeight, baseWidth)); // Skalowanie podstawy do zadanych wymiarów
	drawObjectColor(program, boxContext, baseModel, glm::vec3(1.0f)); // Rysowanie podstawy w kolorze białym

	// R1 - Ramię Rotacji 1
	glm::mat4 arm1Transform = baseTranslate; // Dziedziczenie translacji z podstawy
	arm1Transform = glm::translate(arm1Transform, glm::vec3(0.0, baseHeight, 0.0)); // Przesunięcie ramienia powyżej podstawy
	arm1Transform = glm::rotate(arm1Transform, glm::radians(arm1Angle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotacja wokół osi Y
	glm::mat4 arm1Model = arm1Transform;
	arm1Model = glm::scale(arm1Model, glm::vec3(arm1Width, arm1Length, arm1Width)); // Skalowanie tylko dla tego ramienia
	drawObjectColor(program, boxContext, arm1Model, glm::vec3(1.0, 0.0, 0.0)); // Rysowanie ramienia w kolorze czerwonym

	// Pręt - element łączący
	glm::mat4 pipeTransform = arm1Transform; // Dziedziczenie translacji i rotacji (bez skalowania)
	pipeTransform = glm::translate(pipeTransform, glm::vec3(0.0, arm1Length, 0.0)); // Przesunięcie pręta powyżej ramienia
	glm::mat4 pipeModel = pipeTransform;
	pipeModel = glm::scale(pipeModel, glm::vec3(pipeWidth, pipeHeight, pipeWidth)); // Skalowanie tylko dla pręta
	drawObjectColor(program, boxContext, pipeModel, glm::vec3(0.0, 1.0, 0.0)); // Rysowanie pręta w kolorze zielonym

	// T1 - Ramię Translacji
	glm::mat4 arm2Transform = pipeTransform; // Dziedziczenie translacji i rotacji z pręta
	arm2Transform = glm::translate(arm2Transform, glm::vec3(arm2Width / 2.0, arm2Heigh, 0.0)); // Przesunięcie na szczyt pręta
	arm2Transform = glm::rotate(arm2Transform, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotacja o 90° wokół osi Z
	glm::mat4 arm2Model = arm2Transform;
	arm2Model = glm::scale(arm2Model, glm::vec3(arm2Width, arm2Length, arm2Width)); // Skalowanie tylko dla tego ramienia
	drawObjectColor(program, boxContext, arm2Model, glm::vec3(0.0, 0.0, 1.0)); // Rysowanie w kolorze niebieskim

	// R2 - Ramię Rotacji 2
	glm::mat4 arm3Transform = arm2Transform; // Dziedziczenie translacji i rotacji z poprzedniego ramienia
	arm3Transform = glm::translate(arm3Transform, glm::vec3(0.0, arm2Length - arm2Width / 2.0, 0.0)); // Przesunięcie ramienia
	arm3Transform = glm::rotate(arm3Transform, glm::radians(arm3Angle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotacja wokół osi Z
	glm::mat4 arm3Model = arm3Transform;
	arm3Model = glm::scale(arm3Model, glm::vec3(arm3Width, arm3Length, arm3Width)); // Skalowanie tylko dla tego ramienia
	drawObjectColor(program, boxContext, arm3Model, glm::vec3(1.0, 0.0, 0.0)); // Rysowanie w kolorze czerwonym

	// R3 - Ramię Rotacji 3
	glm::mat4 arm4Transform = arm3Transform; // Dziedziczenie translacji i rotacji z poprzedniego ramienia
	arm4Transform = glm::translate(arm4Transform, glm::vec3(0.0, arm3Length, 0.0)); // Przesunięcie ramienia
	arm4Transform = glm::rotate(arm4Transform, glm::radians(arm4Angle), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotacja wokół osi Y
	glm::mat4 arm4Model = arm4Transform;
	arm4Model = glm::scale(arm4Model, glm::vec3(arm4Width, arm4Length, arm4Width)); // Skalowanie tylko dla tego ramienia
	drawObjectColor(program, boxContext, arm4Model, glm::vec3(0.0, 1.0, 0.0)); // Rysowanie w kolorze zielonym

	// R4 - Ramię Rotacji 4
	glm::mat4 arm5Transform = arm4Transform; // Dziedziczenie translacji i rotacji z poprzedniego ramienia
	arm5Transform = glm::translate(arm5Transform, glm::vec3(0.0, arm4Length, 0.0)); // Przesunięcie ramienia
	arm5Transform = glm::rotate(arm5Transform, glm::radians(arm5Angle), glm::vec3(0.0f, 0.0f, 1.0f)); // Rotacja wokół osi Z
	glm::mat4 arm5Model = arm5Transform;
	arm5Model = glm::scale(arm5Model, glm::vec3(arm5Width, arm5Length, arm5Width)); // Skalowanie tylko dla tego ramienia
	drawObjectColor(program, boxContext, arm5Model, glm::vec3(0.0, 0.0, 1.0)); // Rysowanie w kolorze niebieskim


	glUseProgram(0);
	glfwSwapBuffers(window);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	aspectRatio = width / float(height);
	glViewport(0, 0, width, height);
}

void init(GLFWwindow* window) {
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	glEnable(GL_DEPTH_TEST);
	program = shaderLoader.CreateProgram("shaders/shader.vert", "shaders/shader.frag");


	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

	// Normalne
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
}

void shutdown(GLFWwindow* window) {
	shaderLoader.DeleteProgram(program);
}

const float arm2MaxHeight = 0.15f * pipeHeight;
const float arm2MinHeight = pipeHeight - arm2MaxHeight;

const float arm3MinAngle = -120.0f;
const float arm3MaxAngle = 90.0f;

const float arm5MinAngle = -90.0f;
const float arm5MaxAngle = 90.0f;

void processInput(GLFWwindow* window) {
	float rotationSpeed = 0.5f;
	float translationSpeed = 0.005f;

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		arm1Angle += rotationSpeed; // Obrót w lewo
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		arm1Angle -= rotationSpeed; // Obrót w prawo
	}

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		arm2Heigh += translationSpeed; // Ruch ramienia w dół
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
		arm2Heigh -= translationSpeed; // Ruch ramienia w górę
	}

	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		arm3Angle -= rotationSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		arm3Angle += rotationSpeed;
	}

	if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
		arm4Angle -= rotationSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		arm4Angle += rotationSpeed;
	}

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		arm5Angle -= rotationSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		arm5Angle += rotationSpeed;
	}

	// Ograniczenia dla ruchów
	arm2Heigh = glm::clamp(arm2Heigh, 0.15f * pipeHeight, 0.85f * pipeHeight);
	arm3Angle = glm::clamp(arm3Angle, arm3MinAngle, arm3MaxAngle);
	arm5Angle = glm::clamp(arm5Angle, arm5MinAngle, arm5MaxAngle);
}

void renderLoop(GLFWwindow* window) {
	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		renderScene(window);
		glfwPollEvents();
	}
}