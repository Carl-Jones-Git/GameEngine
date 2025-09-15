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

//#define PC_BUILD
#ifndef PC_BUILD
#include "..\Common\MoveLookController.h"
#endif

// Abstract base class to model mesh objects for rendering in DirectX
class PlayerKart : public Kart {

private:
	XINPUT_STATE controllerState;


public:
#ifndef PC_BUILD
	MoveLookController^ m_controller;
#endif
	PlayerKart(FMOD::System* _FMSystem, ID3D11Device* device, ID3D11DeviceContext* context, Terrain* ground, shared_ptr<Effect>  _perPixelEffect, shared_ptr<Texture> _texture) : Kart(_FMSystem, device,  context,ground,_perPixelEffect, _texture) { /*load(device, filename);*/ }
	~PlayerKart();

	void render(ID3D11DeviceContext *context,int camMode=1);
	void update(ID3D11DeviceContext* context, float stepSize);

	int camMode = 2;

};
