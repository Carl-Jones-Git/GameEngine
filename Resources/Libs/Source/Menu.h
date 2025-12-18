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
#include <DirectXMath.h>

#include <Utils.h>
#include <PlayerKart.h>
#include <Model.h>
#include <Quad.h>
#include <Texture.h>
#include <LookAtCamera.h>

#include <imgui_impl_dx11.h>
#include <imgui.h>
#include "NetMgr.h"

#ifndef PC_BUILD
#include <imgui_impl_uwp.h>
#include "..\Common\DeviceResources.h"
#else
#include <System.h>
#endif

 /// @brief Quality preset levels for graphics rendering
enum class RenderQuality {
	Lowest,
	Low,
	Medium,
	High,
	Highest
};
/// @brief Menu states
enum class MenuState  { 
	Intro, MainMenu, 
	CreateLobby, 
	JoinLobby, 
	WaitingToStart, 
	MenuDone, 
	MenuNone 
};

class Menu
{
protected:
	int quality = (int)RenderQuality::Medium;
	NetMgr *m_networkManager;
#ifdef PC_BUILD
	System* m_system = nullptr;
#else
	std::shared_ptr<DX::DeviceResources> m_system;
#endif

	LookAtCamera* lookAtCamera;
	Quad* backGround = nullptr;
	Texture* backGroundTex = nullptr;
	shared_ptr<Effect> planarShadowEffect= nullptr;
	ID3D11PixelShader* screenQuadPS = nullptr;
	ID3D11VertexShader* screenQuadVS = nullptr;
	ID3D11InputLayout* screenQuadVSInputLayout = nullptr;


	int playerKartWrap = 0;
	vector <std::string> *m_kartWrapNames;
	vector <shared_ptr <Texture>> *m_kartTextures;
	bool mainMenuInit = false;

public:
#ifdef PC_BUILD
	Menu(System* _system) { m_system = _system; };
#else
	Menu(std::shared_ptr<DX::DeviceResources> _system ) { m_system = _system; };
#endif

	void init(NetMgr* _netMgr);
	
	void renderIntro();

	//helper
	void setPlayerKartTextures(vector <std::string> *_kartWraps, vector <shared_ptr <Texture>> *_kartTextures, int _playerKartWrap) {
		playerKartWrap = _playerKartWrap;
		m_kartWrapNames=_kartWraps;
		m_kartTextures=_kartTextures;
	}
	void drawShadow(Model* model, shared_ptr<Effect>  shadowEffect, XMMATRIX* shadowMatrix, ID3D11DeviceContext* context);
	void initMainMenu();
	void renderMainMenu( PlayerKart* m_playerKart, MenuState* MainMenuState,  float m_fixedTimeStep);
	void renderGUIMenu(MenuState* MainMenuState, Kart* m_allKarts[], int nKarts);
	RenderQuality getQuality() { return (RenderQuality)quality; };
	~Menu() {
		cout << "Menu destructor called" << endl;
		if (lookAtCamera)
			delete(lookAtCamera);
		if (backGround)delete(backGround);
		if (backGroundTex)delete(backGroundTex);
		if (screenQuadPS)screenQuadPS->Release();
		if (screenQuadVS)screenQuadVS->Release();
		if (screenQuadVSInputLayout)screenQuadVSInputLayout->Release();
	};
};

