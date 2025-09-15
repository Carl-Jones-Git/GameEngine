#ifndef PROCTERRAIN_H
#define PROCTERRAIN_H

#include <string>
#include <vector>
#include <iostream>
#include <vector>
using namespace std;


#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "PerlinNoise.h"
#include "FloatImage.h"
#include "NoiseImages.h"


#define NUM_TERAIN_MODES 8

struct terrainStruct
{
	glm::vec4 pos;
	glm::vec3 norm;
};


class ProcTerrain{

private:
	////	Shaders - Textures - Models	////
	GLuint terainShader;


	Noise* P = new PerlinNoise();
	FloatImage* terrainImage = nullptr;
	int								terrainWidth, terrainHeight;
	// Multi-fractal coefficients
	float							lacunarity, offset, H;
	float							octaves;
	float							mfScale;
	GLuint							terrainVAO;
	vector<terrainStruct>			terrainVertices;
	vector<int>						terrainIndices;
	//
	// Private API
	//


	void setup();


	//
	// Public API
	//

public:
	ProcTerrain(int mode=4);
	GLuint generateTerain(FloatImage* terrainImage, vector<terrainStruct>* vertices, vector<int>* indices, float tscale, float tyScale);
	int setTerrainMode(int mode);
	void render(glm::mat4 viewProjectionMatrix);
	~ProcTerrain();
	// Size of each "pixel" when mapped to terrain quad
	float	tscale, tyScale;
	void set_tscale(float _tscale) { tscale = _tscale; };
	void set_tyScale(float _tyScale) { tyScale = _tyScale; };
};

#endif