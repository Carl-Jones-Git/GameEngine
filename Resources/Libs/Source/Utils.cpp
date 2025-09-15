/*
 * Copyright (c) 2023 Carl Jones
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "Includes.h"
#include "Utils.h"

using namespace std;

std::wstring StringToWString(string tnameS)
{
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &tnameS[0], (int)tnameS.size(), NULL, 0);
	std::wstring tnameW(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &tnameS[0], (int)tnameS.size(), &tnameW[0], size_needed);
	return tnameW;
}

std::string WStringToString(std::wstring tnameW)
{
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &tnameW[0], (int)tnameW.size(), NULL, 0, NULL, NULL);
	std::string tnameS(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &tnameW[0], (int)tnameW.size(), &tnameS[0], size_needed, NULL, NULL);
	return tnameS;
}
// Helper function to copy cbuffer data from cpu to gpu
HRESULT mapCbuffer(ID3D11DeviceContext *context,void *cBufferCPU, ID3D11Buffer *cBufferGPU,int buffSize)
{
	// Map cBuffer
	D3D11_MAPPED_SUBRESOURCE res;
	HRESULT hr = context->Map(cBufferGPU, 0, D3D11_MAP_WRITE_DISCARD, 0, &res);

	if (SUCCEEDED(hr)) {
		memcpy(res.pData, cBufferCPU, buffSize);
		context->Unmap(cBufferGPU, 0);
	}
	return hr;
}

// from terrain tutorial
// Helper Generates random number between -1.0 and +1.0
float randM1P1()
{	// use srand((unsigned int)time(NULL)); to seed rand()
	float r = (float)((double)rand() / (double)(RAND_MAX)) *2.0f - 1.0f;

	//modified to return a ring with inner radius A and outer radius A+B
	float A = 1, B = 3;
	r *= B; if (r > 0)r += A; else r -= A;
	return r / 6;
}
float rand021()
{	// use srand((unsigned int)time(NULL)); to seed rand()
	float r = (float)((double)rand() / (double)(RAND_MAX));
	return r;
}
// Helper to load a compiled shader
uint32_t LoadShader(const char *filename, char **bytecode)
{

	ifstream	*fp = nullptr;
	uint32_t shaderBytes = -1;
	cout << "loading shader" << endl;

	try
	{
		// Validate parameters
		if (!filename)
			throw exception("loadCSO: Invalid parameters");


		// Open file
		fp = new ifstream(filename, ios::in | ios::binary);

		if (!fp->is_open())
			throw exception("loadCSO: Cannot open file");

		// Get file size
		fp->seekg(0, ios::end);
		shaderBytes = (uint32_t)fp->tellg();

		// Create blob object to store bytecode (exceptions propagate up if any occur)
		cout << "allocating shader memory bytes = " << shaderBytes << endl;
		*bytecode = (char*)malloc(shaderBytes);
		// Read binary data into blob object
		fp->seekg(0, ios::beg);
		fp->read(*bytecode, shaderBytes);

		// Close file and release local resources
		fp->close();
		delete fp;

		// Return DXBlob - ownership implicity passed to caller
		cout << "Done: shader memory bytes = " << shaderBytes << endl;
	}
	catch (exception& e)
	{
		cout << e.what() << endl;

		// Cleanup local resources
		if (fp) {

			if (fp->is_open())
				fp->close();

			delete fp;
		}

		if (bytecode)
			delete bytecode;

		// Re-throw exception
		throw;
	}
	return shaderBytes;
}

