#pragma once
#include <CBufferStructures.h>
#include <locale> 
#include <codecvt>
#include <string>
#include <iostream>
using namespace std;
float randM1P1();
float rand021();
HRESULT mapCbuffer(ID3D11DeviceContext *context, void *cBufferExtSrcL, ID3D11Buffer *cBufferExtL,int buffSize);
uint32_t LoadShader(const char *filename, char **bytecode);
wstring StringToWString(string tnameS);
string WStringToString(wstring tnameW);

