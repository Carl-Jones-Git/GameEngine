#pragma once
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <System.h>
#include <Utils.h>
#include <PlayerKart.h>
#include <Model.h>
#include <Quad.h>
#include <Texture.h>
#include <LookAtCamera.h>
#include "NetMgr.h"
enum Quality {WHITE=-1, LOWEST, LOW, MEDIUM, HIGH, HIGHEST };
enum MenuState { Intro, MainMenu, CreateLobby, JoinLobby, WaitingToStart, MenuDone, MenuNone };
class Menu
{
protected:
	int quality = MEDIUM;
	NetMgr *netMgr;
	System *system = nullptr;
	LookAtCamera* lookAtCamera;
	Quad* backGround = nullptr;
	Texture* backGroundTex = nullptr;
	shared_ptr<Effect> planarShadowEffect= nullptr;
	ID3D11PixelShader* screenQuadPS = nullptr;
	ID3D11VertexShader* screenQuadVS = nullptr;
	//ID3D11SamplerState* sampler = nullptr;
	ID3D11InputLayout* screenQuadVSInputLayout = nullptr;


	int playerKartWrap = 0;
	vector <std::string> *kartWraps;
	vector <shared_ptr <Texture>> *kartTextures;
	bool mainMenuInit = false;

public:
	Menu(System* _system) { system = _system; };

	void init(NetMgr* _netMgr);
	
	void renderIntro();

	//helper
	void setPlayerKartTextures(vector <std::string> *_kartWraps, vector <shared_ptr <Texture>> *_kartTextures, int _playerKartWrap) {
		playerKartWrap = _playerKartWrap;
		kartWraps=_kartWraps;
		kartTextures=_kartTextures;
	}
	void drawShadow(Model* model, shared_ptr<Effect>  shadowEffect, XMMATRIX* shadowMatrix, ID3D11DeviceContext* context);
	void initMainMenu();
	void renderMainMenu( PlayerKart* playerKart, MenuState* MainMenuState,  float stepSize);
	void renderGUIMenu(MenuState* MainMenuState, Kart* kartArray[], int nKarts);
	int getQuality() { return quality; };
	~Menu() {
		cout << "Menu destructor called" << endl;
//		if (planarShadowEffect)delete(planarShadowEffect);
		if (lookAtCamera)
			delete(lookAtCamera);
		if (backGround)delete(backGround);
		if (backGroundTex)delete(backGroundTex);
		if (screenQuadPS)screenQuadPS->Release();
		if (screenQuadVS)screenQuadVS->Release();
		if (screenQuadVSInputLayout)screenQuadVSInputLayout->Release();
	};
};

