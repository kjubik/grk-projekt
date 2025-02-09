#pragma once
#include "glew.h"
#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

#include "../Shader_Loader.h"
#include "../Render_Utils.h"
#include "../Texture.h"
#include "simulation.h"

#include <vector>
#include <random>
#include <numeric>

#include "Terrain.h"

class Boid {
public:
	glm::vec3 position;
	glm::vec3 velocity;
	glm::vec3 scale;
	Core::RenderContext modelContext;
	GLuint textureID;
	SimulationParams* simulationParams;
	ProceduralTerrain* terrain;

	static constexpr float MAX_SPEED = 2.0f;
	static constexpr float MIN_SPEED = 0.2f;

	Boid(glm::vec3 pos, glm::vec3 vel, const Core::RenderContext& context, SimulationParams* simulationPms, ProceduralTerrain* terr, GLuint texID = -1)
		: position(pos), velocity(vel), simulationParams(simulationPms), modelContext(context), terrain(terr), scale(simulationPms->boidModelScale), textureID(texID) {
	}

	void update(float deltaTime, const glm::vec3& acceleration) {
		position += velocity * deltaTime;
		velocity += acceleration * deltaTime;

		float terrainHeight = terrain->getTerrainHeight(position.x, position.z);
		if (position.y < terrainHeight + 0.5f) {
			position.y = terrainHeight + 0.5f;
			velocity.y = glm::abs(velocity.y) * 0.5f;
		}

		applyBounceForceFromBoundingBox(deltaTime);
		limitSpeed();
	}

	void draw(GLuint shaderProgram, GLuint modelLoc, const glm::mat4& view,
		const glm::mat4& projection, GLuint viewLoc, GLuint projectionLoc, glm::vec3 cameraPos)
	{
		glUseProgram(shaderProgram);
		Core::SetActiveTexture(textureID, "boidTexture", shaderProgram, 0);

		GLint objectColorLoc = glGetUniformLocation(shaderProgram, "objectColor");
		glUniform3f(objectColorLoc, 0.7f, 0.7f, 0.7f);

		GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
		glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));

		GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
		glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

		GLint lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
		glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

		glm::mat4 rotationMatrix = getRotationMatrixFromVelocity(velocity);
		glm::mat4 model = glm::translate(glm::mat4(1.0f), position) *
			rotationMatrix *
			glm::scale(glm::mat4(1.0f), scale);

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

		if (position.x < simulationParams->boundMin) {
			bounceForce.x += simulationParams->bounceForce * (simulationParams->boundMin - position.x);
		}
		else if (position.x > simulationParams->boundMax) {
			bounceForce.x -= simulationParams->bounceForce * (position.x - simulationParams->boundMax);
		}

		if (position.y < simulationParams->boundMin) {
			bounceForce.y += simulationParams->bounceForce * (simulationParams->boundMin - position.y);
		}
		else if (position.y > simulationParams->boundMax) {
			bounceForce.y -= simulationParams->bounceForce * (position.y - simulationParams->boundMax);
		}

		if (position.z < simulationParams->boundMin) {
			bounceForce.z += simulationParams->bounceForce * (simulationParams->boundMin - position.z);
		}
		else if (position.z > simulationParams->boundMax) {
			bounceForce.z -= simulationParams->bounceForce * (position.z - simulationParams->boundMax);
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
	SimulationParams* simulationParams;
	ProceduralTerrain* terrain;
	Flock() {}

	Flock(SimulationParams* simulParams, ProceduralTerrain* terr, const Core::RenderContext& modelContext, GLuint gradientTextures[10]) {
		simulationParams = simulParams;
		terrain = terr;

		for (int i = 0; i < simulationParams->boidNumber; ++i) {
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

			GLuint randomTextureID = gradientTextures[rand() % 10];

			boids.emplace_back(position, velocity, modelContext, simulationParams, terrain, randomTextureID);
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

	void draw(GLuint shaderProgram, GLuint modelLoc, const glm::mat4& view, const glm::mat4& projection, GLuint viewLoc, GLuint projectionLoc, glm::vec3 cameraPos) {
		for (auto& boid : boids) {
			boid.draw(shaderProgram, modelLoc, view, projection, viewLoc, projectionLoc, cameraPos);
		}
	}
private:
	glm::vec3 computeAvoidance(const Boid& boid) {
		glm::vec3 avoidance(0.0f);
		int count = 0;

		for (const auto& other : boids) {
			if (&other == &boid) continue;

			float distance = glm::length(boid.position - other.position);
			if (distance < simulationParams->avoidRadius && distance > 0.0f) {
				avoidance += glm::normalize(boid.position - other.position) / (distance * distance);
				count++;
			}
		}

		if (count > 0) {
			avoidance /= static_cast<float>(count);
			avoidance *= simulationParams->avoidForce;
		}
		return avoidance;
	}

	glm::vec3 computeAlignment(const Boid& boid) {
		glm::vec3 averageVelocity(0.0f);
		int count = 0;

		for (const auto& other : boids) {
			if (&other == &boid) continue;

			float distance = glm::length(boid.position - other.position);
			if (distance < simulationParams->alignRadius) {
				averageVelocity += other.velocity;
				count++;
			}
		}

		if (count > 0) {
			averageVelocity /= static_cast<float>(count);
			averageVelocity = glm::normalize(averageVelocity) * simulationParams->alignForce;
		}
		return averageVelocity;
	}

	glm::vec3 computeCohesion(const Boid& boid) {
		glm::vec3 centerOfMass(0.0f);
		int count = 0;

		for (const auto& other : boids) {
			if (&other == &boid) continue;

			float distance = glm::length(boid.position - other.position);
			if (distance < simulationParams->cohesionRadius) {
				centerOfMass += other.position;
				count++;
			}
		}

		if (count > 0) {
			centerOfMass /= static_cast<float>(count);
			glm::vec3 cohesionForce = glm::normalize(centerOfMass - boid.position) * simulationParams->cohesionForce;
			return cohesionForce;
		}

		return glm::vec3(0.0f);
	}
};
