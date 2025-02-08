#pragma once
#ifndef UTILS_H
#define UTILS_H

#include "glew.h"

#include <GLFW/glfw3.h>
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>

#include <cstdlib>
#include "boids/vertices.h"

void setupBoidVAOandVBO(GLuint& VAO, GLuint& VBO, const float* vertices, size_t size) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void setupBoundingBox(GLuint& boundingBoxVAO, GLuint& boundingBoxVBO, GLuint& boundingBoxEBO) {
    glGenVertexArrays(1, &boundingBoxVAO);
    glGenBuffers(1, &boundingBoxVBO);
    glGenBuffers(1, &boundingBoxEBO);

    glBindVertexArray(boundingBoxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, boundingBoxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(boundingBoxVertices), boundingBoxVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, boundingBoxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(boundingBoxIndices), boundingBoxIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void drawBoundingBox(glm::mat4 view, glm::mat4 projection, GLuint lineShader, GLuint& boundingBoxVAO) {
    glUseProgram(lineShader);

    glm::mat4 model = glm::mat4(1.0f);
    GLuint modelLoc = glGetUniformLocation(lineShader, "model");
    GLuint viewLoc = glGetUniformLocation(lineShader, "view");
    GLuint projectionLoc = glGetUniformLocation(lineShader, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(boundingBoxVAO); // mucho importante
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0); // mucho importante
    glBindVertexArray(0);
}

#endif //UTILS_H
