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

#include <vector>
#include <random>
#include <numeric>

class PerlinNoise {
private:
	std::vector<int> p;

	static float fade(float t) {
		return t * t * t * (t * (t * 6 - 15) + 10);
	}

	static float lerp(float t, float a, float b) {
		return a + t * (b - a);
	}

	static float grad(int hash, float x, float y, float z) {
		int h = hash & 15;
		float u = h < 8 ? x : y;
		float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
		return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
	}

public:
	PerlinNoise() {
		p.resize(256);
		std::iota(p.begin(), p.end(), 0);
		std::shuffle(p.begin(), p.end(), std::default_random_engine());

		p.insert(p.end(), p.begin(), p.end());
	}

	float noise(float x, float y, float z) const {
		int X = (int)floor(x) & 255;
		int Y = (int)floor(y) & 255;
		int Z = (int)floor(z) & 255;

		x -= floor(x);
		y -= floor(y);
		z -= floor(z);

		float u = fade(x);
		float v = fade(y);
		float w = fade(z);

		int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z;
		int B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

		return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
			lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
			lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1), grad(p[BA + 1], x - 1, y, z - 1)),
				lerp(u, grad(p[AB + 1], x, y - 1, z - 1), grad(p[BB + 1], x - 1, y - 1, z - 1))));
	}
};

class ProceduralTerrain {
private:
	std::vector<glm::vec3> vertices;
	std::vector<GLuint> indices;
	GLuint terrainVAO, terrainVBO, terrainEBO;
	std::vector<std::vector<float>> heightMap;
	float planeSize;
	int resolution;
	PerlinNoise perlinNoise;
public:
	bool wireframeOnlyView = false;
	ProceduralTerrain(float size = 10.0f, int res = 10)
		: planeSize(size), resolution(res) {
		generateTerrain();
		setupMesh();
	}

	void generateTerrain() {
		vertices.clear();
		indices.clear();

		float frequency = .1f; // ammount of peeks
		float heightScale = 10.0f; // height of peeks
		heightMap.resize(resolution + 1, std::vector<float>(resolution + 1));

		// Generate vertices
		for (int z = 0; z <= resolution; ++z) {
			for (int x = 0; x <= resolution; ++x) {
				float xPos = (x / static_cast<float>(resolution)) * planeSize - (planeSize / 2.0f);
				float zPos = (z / static_cast<float>(resolution)) * planeSize - (planeSize / 2.0f);

				// Apply Perlin noise with scaling
				float noise = perlinNoise.noise(xPos * frequency, 1.0f, zPos * frequency);
				noise = (noise + 1.0f) / 2.0f * heightScale;

				vertices.push_back(glm::vec3(xPos, noise, zPos));
				heightMap[z][x] = noise;  // Store the height
			}
		}

		for (int z = 0; z < resolution; ++z) {
			for (int x = 0; x < resolution; ++x) {
				int topLeft = z * (resolution + 1) + x;
				int topRight = topLeft + 1;
				int bottomLeft = (z + 1) * (resolution + 1) + x;
				int bottomRight = bottomLeft + 1;

				/*
					0---1
					|  /|
					| / |
					|/  |
					2---3
				*/

				// First triangle (0-1-2)
				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(topRight);

				// Second triangle (1-3-2)
				indices.push_back(topRight);
				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);
			}
		}
	}

	void setupMesh() {
		glGenVertexArrays(1, &terrainVAO);
		glBindVertexArray(terrainVAO);

		glGenBuffers(1, &terrainVBO);
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3),
			vertices.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &terrainEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint),
			indices.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
		glEnableVertexAttribArray(0);

		glBindVertexArray(0);
	}

	void render(GLuint shaderProgram, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model) {
		glUseProgram(shaderProgram);

		float minHeight = std::numeric_limits<float>::max();
		float maxHeight = std::numeric_limits<float>::lowest();

		for (const auto& vertex : vertices) {
			float height = vertex.y;
			minHeight = std::min(minHeight, height);
			maxHeight = std::max(maxHeight, height);
		}

		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
		GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLint modelLoc = glGetUniformLocation(shaderProgram, "model");

		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		GLint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

		glBindVertexArray(terrainVAO);

		if (wireframeOnlyView) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glLineWidth(2.0f);
			glUniform3f(colorLoc, 0.8f, 0.0f, 0.8f);
			glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			for (size_t i = 0; i < indices.size() / 3; ++i) {
				unsigned int index1 = indices[i * 3];
				unsigned int index2 = indices[i * 3 + 1];
				unsigned int index3 = indices[i * 3 + 2];

				glm::vec3 vertex1 = vertices[index1];
				glm::vec3 vertex2 = vertices[index2];
				glm::vec3 vertex3 = vertices[index3];

				float avgHeight = (vertex1.y + vertex2.y + vertex3.y) / 3.0f;

				float normalizedHeight = (avgHeight - minHeight) / (maxHeight - minHeight);
				normalizedHeight = glm::clamp(normalizedHeight, 0.0f, 1.0f);
				glm::vec3 color = glm::vec3(normalizedHeight);

				// TODO: use different textures based on normalizedHeight

				glUniform3f(colorLoc, color.x, color.y, color.z);
				glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(i * 3 * sizeof(unsigned int)));
			}
		}
	}


	void translateTerrain(const glm::vec3& offset) {
		// Apply translation to each vertex
		for (auto& vertex : vertices) {
			vertex += offset;  // Apply the translation to x, y, and z
		}

		// Update the heightMap only for the Y-offset (vertical movement)
		if (offset.y != 0.0f) {
			for (auto& row : heightMap) {
				for (auto& height : row) {
					height += offset.y;  // Adjust only the height values
				}
			}
		}

		// Update the VBO with new vertex positions
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), vertices.data());
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	float getTerrainHeight(float x, float z) {
		float gridX = (x + planeSize / 2.0f) / planeSize * resolution;
		float gridZ = (z + planeSize / 2.0f) / planeSize * resolution;

		int x0 = static_cast<int>(floor(gridX));
		int z0 = static_cast<int>(floor(gridZ));
		int x1 = x0 + 1;
		int z1 = z0 + 1;

		if (x0 < 0 || z0 < 0 || x1 >= resolution + 1 || z1 >= resolution + 1)
			return -std::numeric_limits<float>::infinity();

		float h00 = heightMap[z0][x0];
		float h10 = heightMap[z0][x1];
		float h01 = heightMap[z1][x0];
		float h11 = heightMap[z1][x1];

		float dx = gridX - x0;
		float dz = gridZ - z0;

		float height = (1 - dx) * (1 - dz) * h00 +
			dx * (1 - dz) * h10 +
			(1 - dx) * dz * h01 +
			dx * dz * h11;

		return height;
	}

	~ProceduralTerrain() {
		glDeleteVertexArrays(1, &terrainVAO);
		glDeleteBuffers(1, &terrainVBO);
		glDeleteBuffers(1, &terrainEBO);
	}
};
