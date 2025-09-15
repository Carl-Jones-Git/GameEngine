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

// Used to customise compilation for PC define PC_BUILD for UWP(XBoxOne) comment //define PC_BUILD.
#define PC_BUILD

#pragma once
#ifdef PC_BUILD
#include <targetver.h>
//wstring ResourcePath = L"..\\";
//string ShaderPath = "Shaders\\cso\\";
#else
#include <agile.h>
//wstring ResourcePath = L"Assets\\";
//string ShaderPath = "";
#endif // DEBUG


// Windows Header Files
// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#define DISABLE_CUDA_PHYSX
#include <windows.h>

// C RunTime and STL Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <cstdint>
#include <conio.h>
#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <exception>


// Core types
#include <CGDClock.h>
#include <CGDConsole.h>
//#include <System.h>

// DirectX headers
#include <d3d11_2.h>
#include <dxgi1_3.h>
#include <d2d1_1.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>
#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <memory>
#include <concrt.h>
#include <iostream>
#include <fstream>
#include <functional>

using namespace std;
using namespace DirectX::PackedVector;
using DirectX::XMQuaternionRotationRollPitchYaw;
using DirectX::XMQuaternionSlerp;
using DirectX::XMMatrixRotationQuaternion;
using DirectX::XMMatrixRotationZ;
using DirectX::XMMatrixRotationY;
using DirectX::XMMatrixRotationX;
using DirectX::XMMatrixScaling;
using DirectX::XMMatrixTranslation;
using DirectX::XMConvertToRadians;
using DirectX::XMMatrixIdentity;
using DirectX::XMVectorZero;
using DirectX::XMVectorSet;
using DirectX::XMVectorGetX;
using DirectX::XMVectorGetY;
using DirectX::XMVectorGetZ;
using DirectX::XMVECTOR;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT2;
using DirectX::XMMATRIX;
using DirectX::XMFLOAT2;
using DirectX::XMFLOAT3;
using DirectX::XM_PI;
using DirectX::XMMatrixPerspectiveFovLH;
using DirectX::XMVectorSet;
using DirectX::XMVECTOR;
using DirectX::XMVectorZero;
using DirectX::XMVectorAdd;
using DirectX::XMVectorMultiply;
using DirectX::XMMatrixTranslation;
using DirectX::XMMatrixScaling;
