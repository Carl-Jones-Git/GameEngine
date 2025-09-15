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
#include "TextureLoader.h"


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
	GLuint diffuseTexture;
	string diffusePath = "..\\..\\Resources\\Textures\\BrightonKarting5.bmp";
	string heightPath = "..\\..\\Resources\\Levels\\Terrain01.bmp";

	unsigned char* difBuffer = nullptr;
	int difWidth;
	int difHeight;
	int difChannels = 1;

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
	float	tscale = 0.1953125, tyScale;
	int		terrainWidth, terrainHeight;
	void set_tscale(float _tscale) { tscale = _tscale; };
	void set_tyScale(float _tyScale) { tyScale = _tyScale; };
	int LoadPNG();
	float CalculateYValue(float x, float z);
};

#endif