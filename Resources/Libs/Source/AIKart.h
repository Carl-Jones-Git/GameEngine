
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

#pragma once

#include <d3d11_2.h>
#include <Effect.h>
#include <Material.h>
#include "Model.h"
#include <Effect.h>
#include <Kart.h>
#include <Texture.h>
#include <Xinput.h>
#include <FmodStudio\inc\fmod.hpp>
#include <TinyXML\Tinyxml2.h>
namespace ti = tinyxml2;
#define MAX_NAV_POINTS 100
XMVECTOR vecFromStr(string& str);


class NavPoints
{
public:
	//Public Attributes to be made protected with accessor methods
	//Prefereable to use vector 
	XMVECTOR pos[MAX_NAV_POINTS];
	float speed[MAX_NAV_POINTS];
	float rad[MAX_NAV_POINTS];
	int numNavPoints = 0;
	void load(string path,float mapScale = 2.0f, float LHCoords = -1);
};

// Derived from base class Kart
class AIKart : public Kart {
private:
	const double TIME_OUT = 5.0;//*FPS;
	NavPoints *navPoints = nullptr;
	int currentNavPoint = 0;
	double countDown = TIME_OUT;
	physx::PxVec3 oldPos;

public:
	AIKart(FMOD::System* _FMSystem, ID3D11Device* device, ID3D11DeviceContext* context, Terrain* ground, NavPoints* navPoints, shared_ptr<Effect> _perPixelEffect,  shared_ptr<Texture> _texture = nullptr) : Kart(_FMSystem, device, context,ground, _perPixelEffect,  _texture) { init( navPoints); /*load(device, filename);*/ }
	~AIKart();
	void init(NavPoints* navPoints);
	void update(ID3D11DeviceContext* context, float stepSize);
};
