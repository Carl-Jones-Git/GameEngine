#include "ProcTerrain.h"
#include "ShaderLoader.h"

using namespace std;


//
// Private API
//

void ProcTerrain::setup()
{
	GLSL_ERROR glsl_error = ShaderLoader::createShaderProgram(
		string("..\\..\\Resources\\Shaders\\Terrain_shader.vert"),
		string("..\\..\\Resources\\Shaders\\Terrain_shader.frag"), &terainShader);


	// Setup example terrain
	terrainWidth = terrainHeight = 512;
	tscale = 0.2f; // size of each "pixel" when mapped to terrain quad
	tyScale = 20.0f;

	lacunarity = 2.0f;
	offset = 0.0f;
	H = 0.8f;
	octaves = 2.0f;
	mfScale = 0.01f;

	//terrainImage = waveModulation(P, terrainWidth, terrainHeight, 0.01f, 2.0f, glm::vec2(0.0f, 0.0f), glm::pi<float>() * 4.5f);

	//// Map to [-1, 1] range
	//terrainImage->normalise();
	//// Maps [-1, 1] to [0, 1]
	//terrainImage->eval([](int x, int y, float value)->float {return (value + 1.0f) / 2.0f; });
	//terrainVAO = generateTerain(terrainImage, &terrainVertices, &terrainIndices, tscale, tyScale);
}
void ProcTerrain::render(glm::mat4 viewProjectionMatrix)
{
	//set up for terrain rendering
	glUseProgram(terainShader);
	glUniform1f(glGetUniformLocation(terainShader, "tscale"), tscale);
	glUniform1f(glGetUniformLocation(terainShader, "tyScale"), tyScale);
	glBindVertexArray(terrainVAO);
	glm::mat4 model = glm::translate(glm::mat4(1.0), glm::vec3(-terrainWidth / 2 * tscale, -(tyScale / 2), -terrainWidth / 2 * tscale));
	glm::mat4 mvpMatrix = viewProjectionMatrix * model;
	glUniformMatrix4fv(glGetUniformLocation(terainShader, "mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glDrawElements(GL_TRIANGLE_STRIP, (terrainImage->w * terrainImage->h) + (terrainImage->w - 1) * (terrainImage->h - 2), GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
	glUseProgram(0);
}
int ProcTerrain::setTerrainMode(int terain_mode)
{
	if (terain_mode == 0)
		terrainImage = perlinNoiseImage(P, terrainWidth, terrainHeight, 0.01f);
	else if (terain_mode == 1)
		terrainImage = fnImage(P, terrainWidth, terrainHeight, 0.01f, 2.0f);
	else if (terain_mode == 2)
		terrainImage = snImage(P, terrainWidth, terrainHeight, 0.001f, 0.5f, 0.5f);
	else if (terain_mode == 3)
		terrainImage = turbulence(P, terrainWidth, terrainHeight, 0.01f, 0.5f);
	else if (terain_mode == 4)
		terrainImage = waveModulation(P, terrainWidth, terrainHeight, 0.01f, 2.0f, glm::vec2(0.0f, 0.0f), glm::pi<float>() * 4.5f);
	else if (terain_mode == 5)
		terrainImage = multifractal_dh1(P, terrainWidth, terrainHeight, 8.0, 2.0, 0.7, 0.1, 0.0075);
	else if (terain_mode == 6)
		terrainImage = multifractal_dhR1(P, terrainWidth, terrainHeight, 8.0, 2.0, 1.0, 1.0, 2.0f, 0.005);
	else if (terain_mode == 7)
		terrainImage = fBM(P, terrainWidth, terrainHeight, octaves, lacunarity, H, mfScale);


	// Map to [-1, 1] range
	terrainImage->normalise();
	// Maps [-1, 1] to [0, 1]
	terrainImage->eval([](int x, int y, float value)->float {return (value + 1.0f) / 2.0f; });
	terrainVAO = generateTerain(terrainImage, &terrainVertices, &terrainIndices, tscale, tyScale);
	return terain_mode;
}



GLuint ProcTerrain::generateTerain(FloatImage* terrainImage, vector<terrainStruct>* vertices, vector<int>* indices, float tscale, float tyScale)
{
	vertices->clear();
	indices->clear();
	vertices->reserve(terrainImage->h * terrainImage->w);
	indices->reserve((terrainImage->w * terrainImage->h) + (terrainImage->w - 1) * (terrainImage->h - 2));

	int tWidth = terrainImage->w;
	int tHeight = terrainImage->h;

	//Create a regular grid for the vertices matching width and heigh of the heigh image
	for (int z = 0; z < tHeight; z++)
	{
		for (int x = 0; x < tWidth; x++)
		{

			float L = *(terrainImage->data + (z * tWidth) + max(x - 1, 0));
			float R = *(terrainImage->data + (z * tWidth) + min(x + 1, tWidth - 1));
			float T = *(terrainImage->data + (max(z - 1, 0) * tWidth) + x);
			float B = *(terrainImage->data + (min(z + 1, tHeight - 1) * tWidth) + x);;
			glm::vec3 normal = glm::vec3(2.0f * (R - L), 4.0f / 128.0, -2.0f * (B - T));
			normal = glm::normalize(normal);
			float* y = terrainImage->data + (z * tWidth) + x;
			vertices->push_back({ glm::vec4(x, *y, z, 1.0), normal });
			//cout << normal.x << normal.z << normal.y << endl;
		}
	}

	//Create indices
	for (int row = 0; row < tHeight - 1; row++)
	{
		if (row % 2 == 0) { // even rows
			for (int col = 0; col < tWidth; col++)
			{
				indices->push_back(col + row * tWidth);
				indices->push_back(col + (row + 1) * tWidth);
			}
		}
		else
		{ // odd rows
			for (int col = tWidth - 1; col > 0; col--)
			{
				indices->push_back(col + (row + 1) * tWidth);
				indices->push_back(col - 1 + +row * tWidth);
			}
		}
	}

	//Setup the VAO for terrain mesh
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, terrainImage->h * terrainImage->w * sizeof(terrainStruct), vertices->data(), GL_STATIC_DRAW);

	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(int), indices->data(), GL_STATIC_DRAW);


	// vertex Positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(terrainStruct), (void*)0);
	// vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(terrainStruct), (void*)offsetof(terrainStruct, norm));

	glBindVertexArray(0);

	return VAO;
}





ProcTerrain::ProcTerrain(int mode)
{
	setup();
	setTerrainMode(mode);
}




ProcTerrain::~ProcTerrain() {

}




