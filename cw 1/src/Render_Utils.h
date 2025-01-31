#pragma once
#include "glew.h"

namespace Core
{
	// VAO - zmienna zawierajaca wskaznik do VAO
	// numVertices - liczba wierzcholkow do narysowania
	void drawVAO(GLuint VAO, int numVertices);

	// VAO - zmienna zawierajaca wskaznik do VAO
	// numIndexes - liczba indeksow do naryowania
	void drawVAOIndexed(GLuint VAO, int numVertices);


	// vertexArray - jednowymiarowa tablica zawierajaca wartosci opisujace pozycje kolejnych wierzcholkow w jednym ciagu (x1, y1, z1, w1, x2, y2, z2, w2, ...)
	// numVertices - liczba wierzcholkow do narysowania
	// elementSize - liczba wartosci opisujacych pojedynczy wierzcholek (np. 3 gdy wierzcholek opisany jest trojka (x, y, z))
	GLuint initVAO(const float* vertexArray, int numVertices, int elementSize);


	// indexArray - jednowymiarowa tablica zawierajaca indeksy wierzcholkow kolejnych trojkatow w jednym ciagu (t1_i1, t1_i2, t1_i3, t2_i1, t2_i2, t2_i3, ...)
	// numIndexes - liczba indeksow w tablicy indexArray
	GLuint initVAOIndexed(const float* vertexArray, const unsigned int* indexArray, int numVertices, int elementSize, int numIndexes);

	void deleteVAO(GLuint VAO);
}