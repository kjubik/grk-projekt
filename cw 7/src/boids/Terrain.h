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
	std::vector<glm::vec3> normals;
	std::vector<glm::vec3> tangents;
	std::vector<glm::vec3> bitangents;
	std::vector<GLuint> indices;
	GLuint terrainVAO, terrainVBO, terrainEBO;
	std::vector<std::vector<float>> heightMap;
	float planeSize;
	int resolution;
	PerlinNoise perlinNoise;
	std::vector<glm::vec2> uvs;

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
		uvs.clear();
		tangents.clear();
		bitangents.clear();
		normals.clear();

		float frequency = .1f;
		float heightScale = 10.0f;
		heightMap.resize(resolution + 1, std::vector<float>(resolution + 1));

		for (int z = 0; z <= resolution; ++z) {
			for (int x = 0; x <= resolution; ++x) {
				float xPos = (x / static_cast<float>(resolution)) * planeSize - (planeSize / 2.0f);
				float zPos = (z / static_cast<float>(resolution)) * planeSize - (planeSize / 2.0f);

				float noise = perlinNoise.noise(xPos * frequency, 1.0f, zPos * frequency);
				noise = (noise + 1.0f) / 2.0f * heightScale;

				vertices.push_back(glm::vec3(xPos, noise, zPos));
				heightMap[z][x] = noise;

				float u = x / static_cast<float>(resolution);
				float v = z / static_cast<float>(resolution);
				uvs.push_back(glm::vec2(u, v));

				normals.push_back(glm::vec3(0.0f));
				tangents.push_back(glm::vec3(0.0f));
				bitangents.push_back(glm::vec3(0.0f));
			}
		}

		for (int z = 0; z < resolution; ++z) {
			for (int x = 0; x < resolution; ++x) {
				int topLeft = z * (resolution + 1) + x;
				int topRight = topLeft + 1;
				int bottomLeft = (z + 1) * (resolution + 1) + x;
				int bottomRight = bottomLeft + 1;

				indices.push_back(topLeft);
				indices.push_back(bottomLeft);
				indices.push_back(topRight);

				indices.push_back(topRight);
				indices.push_back(bottomLeft);
				indices.push_back(bottomRight);
			}
		}

		for (size_t i = 0; i < indices.size(); i += 3) {
			GLuint i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
			glm::vec3 v0 = vertices[i0], v1 = vertices[i1], v2 = vertices[i2];
			glm::vec2 uv0 = uvs[i0], uv1 = uvs[i1], uv2 = uvs[i2];

			glm::vec3 deltaPos1 = v1 - v0;
			glm::vec3 deltaPos2 = v2 - v0;
			glm::vec2 deltaUV1 = uv1 - uv0;
			glm::vec2 deltaUV2 = uv2 - uv0;

			float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x + 1e-6);
			glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
			glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
			glm::vec3 normal = glm::normalize(glm::cross(deltaPos1, deltaPos2));

			tangents[i0] += tangent;
			tangents[i1] += tangent;
			tangents[i2] += tangent;

			bitangents[i0] += bitangent;
			bitangents[i1] += bitangent;
			bitangents[i2] += bitangent;

			normals[i0] += normal;
			normals[i1] += normal;
			normals[i2] += normal;
		}

		for (size_t i = 0; i < vertices.size(); ++i) {
			normals[i] = glm::normalize(normals[i]);
			tangents[i] = glm::normalize(tangents[i]);
			bitangents[i] = glm::normalize(bitangents[i]);
		}
	}


	void setupMesh() {
		glGenVertexArrays(1, &terrainVAO);
		glBindVertexArray(terrainVAO);

		glGenBuffers(1, &terrainVBO);
		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);

		std::vector<float> vertexData;
		for (size_t i = 0; i < vertices.size(); ++i) {
			vertexData.push_back(vertices[i].x);
			vertexData.push_back(vertices[i].y);
			vertexData.push_back(vertices[i].z);
			vertexData.push_back(uvs[i].x);
			vertexData.push_back(uvs[i].y);
			vertexData.push_back(normals[i].x);
			vertexData.push_back(normals[i].y);
			vertexData.push_back(normals[i].z);
			vertexData.push_back(tangents[i].x);
			vertexData.push_back(tangents[i].y);
			vertexData.push_back(tangents[i].z);
			vertexData.push_back(bitangents[i].x);
			vertexData.push_back(bitangents[i].y);
			vertexData.push_back(bitangents[i].z);
		}
		glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

		glGenBuffers(1, &terrainEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

		GLsizei stride = 14 * sizeof(float);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
		glEnableVertexAttribArray(0);

		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
		glEnableVertexAttribArray(3);

		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
		glEnableVertexAttribArray(4);

		glBindVertexArray(0);
	}

	void render(GLuint shaderProgram, const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model,
		GLuint textureID, GLuint normalMapID, GLuint shadowMap,
		glm::vec3 cameraPos, glm::vec3 lightPos, glm::mat4 lightSpaceMatrix)
	{
		glUseProgram(shaderProgram);

		GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
		GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
		GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
		GLint lightPosLoc = glGetUniformLocation(shaderProgram, "lightPos");
		GLint lightColLoc = glGetUniformLocation(shaderProgram, "lightColor");
		GLint viewPosLoc = glGetUniformLocation(shaderProgram, "viewPos");
		GLint lightSpaceLoc = glGetUniformLocation(shaderProgram, "lightSpaceMatrix");

		glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
		glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
		glUniform3fv(lightColLoc, 1, glm::value_ptr(glm::vec3(1.0f))); // White light color

		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(lightSpaceLoc, 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

		Core::SetActiveTexture(textureID, "terrainTexture", shaderProgram, 0);
		Core::SetActiveTexture(normalMapID, "normalMap", shaderProgram, 1);
		Core::SetActiveTexture(shadowMap, "shadowMap", shaderProgram, 2);

		glBindVertexArray(terrainVAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}


	void translateTerrain(const glm::vec3& offset) {
		for (size_t i = 0; i < vertices.size(); ++i) {
			vertices[i] += offset;

			uvs[i].x += offset.x / planeSize;
			uvs[i].y += offset.z / planeSize;
		}

		if (offset.y != 0.0f) {
			for (auto& row : heightMap) {
				for (auto& height : row) {
					height += offset.y;
				}
			}
		}

		glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);

		std::vector<float> vertexData;
		for (size_t i = 0; i < vertices.size(); ++i) {
			vertexData.push_back(vertices[i].x);
			vertexData.push_back(vertices[i].y);
			vertexData.push_back(vertices[i].z);
			vertexData.push_back(uvs[i].x);
			vertexData.push_back(uvs[i].y);
			vertexData.push_back(normals[i].x);
			vertexData.push_back(normals[i].y);
			vertexData.push_back(normals[i].z);
			vertexData.push_back(tangents[i].x);
			vertexData.push_back(tangents[i].y);
			vertexData.push_back(tangents[i].z);
			vertexData.push_back(bitangents[i].x);
			vertexData.push_back(bitangents[i].y);
			vertexData.push_back(bitangents[i].z);
		}

		glBufferSubData(GL_ARRAY_BUFFER, 0, vertexData.size() * sizeof(float), vertexData.data());
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
