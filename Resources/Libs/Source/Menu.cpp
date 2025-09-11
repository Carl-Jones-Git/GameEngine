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

#include <Includes.h>
#include "Menu.h"
extern wstring ResourcePath;
extern string ShaderPath;
void Menu::init(NetMgr* _netMgr)
{ 
	netMgr = _netMgr;
	ID3D11DeviceContext* context = system->GetD3DDeviceContext();

	char* tmpShaderBytecode = nullptr;

	SIZE_T shaderBytes = LoadShader((ShaderPath + "basic_texture_vs.cso").c_str(), &tmpShaderBytecode);
	system->GetD3DDevice()->CreateVertexShader(tmpShaderBytecode, shaderBytes, NULL, &screenQuadVS);
	system->GetD3DDevice()->CreateInputLayout(basicVertexDesc, ARRAYSIZE(basicVertexDesc), tmpShaderBytecode, shaderBytes, &screenQuadVSInputLayout);
	free(tmpShaderBytecode);

	shaderBytes = LoadShader((ShaderPath + "basic_texture_ps.cso").c_str(), &tmpShaderBytecode);
	system->GetD3DDevice()->CreatePixelShader(tmpShaderBytecode, shaderBytes, NULL, &screenQuadPS);
	free(tmpShaderBytecode);

	backGroundTex = new Texture(system->GetD3DDevice(), ResourcePath+L"Resources\\Textures\\Intro02.png");
	ID3D11ShaderResourceView* mytexSRV = backGroundTex->getShaderResourceView();
	context->PSSetShaderResources(0, 1, &mytexSRV);
	context->PSSetShaderResources(1, 1, &mytexSRV);

	// Set depth copy vertex and pixel shaders
	context->VSSetShader(screenQuadVS, 0, 0);
	context->PSSetShader(screenQuadPS, 0, 0);

	backGround = new Quad(system->GetD3DDevice(), screenQuadVSInputLayout);
};


void Menu::renderIntro()
{
	ID3D11DeviceContext* context = system->GetD3DDeviceContext();
	// Clear the screen
	static const FLOAT clearColor[4] = { 0.6f, 0, 0, 1.0f };
	context->ClearRenderTargetView(system->GetBackBufferRenderTargetView(), clearColor);
	context->ClearDepthStencilView(system->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	backGround->render(context);
	
	system->Present();
}


//helper
void Menu::drawShadow(Model* model, shared_ptr<Effect>  shadowEffect, XMMATRIX* shadowMatrix, ID3D11DeviceContext* context)
{
	XMMATRIX  modelMatrix = model->getWorldMatrix();
	shared_ptr<Effect>  modelEffect = model->getEffect();
	model->setEffect(shadowEffect);
	model->setWorldMatrix(modelMatrix * *shadowMatrix);

	model->update(context);
	model->render(context);

	model->setWorldMatrix(modelMatrix);
	model->update(context);
	model->setEffect(modelEffect);
};

void Menu::initMainMenu()
{
	ID3D11Device* device = system->GetD3DDevice();
	lookAtCamera = new LookAtCamera(device, XMVectorSet(-1.5, -0.4, 1.5, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));

	ImGuiStyle& style = ImGui::GetStyle();
	style.TabRounding = 8.f;
	style.FrameRounding = 8.f;
	style.GrabRounding = 8.f;
	style.WindowRounding = 8.f;
	style.PopupRounding = 8.f;
	style.ChildRounding = 8.f;
	style.WindowBorderSize = 0.0f;
	if (backGroundTex)
		delete(backGroundTex);
	backGroundTex = nullptr;
	backGroundTex = new Texture(system->GetD3DDevice(), ResourcePath+ L"Resources\\Textures\\Garage04.png");
	planarShadowEffect = make_shared<Effect>(system->GetD3DDevice(), (ShaderPath + "per_pixel_lighting_vs.cso").c_str(), (ShaderPath + "planarShadow_ps.cso").c_str(), extVertexDesc, ARRAYSIZE(extVertexDesc));

	ID3D11RasterizerState* rState = planarShadowEffect->getRasterizerState();//this just gets the default state
	D3D11_RASTERIZER_DESC rSDesc;
	rState->GetDesc(&rSDesc);//this just gets the default state description
	rSDesc.CullMode = D3D11_CULL_NONE;
	rState->Release();
	system->GetD3DDevice()->CreateRasterizerState(&rSDesc, &rState);
	planarShadowEffect->setRasterizerState(rState);

	// Setup Depth and Stencil states
	auto updateSState = planarShadowEffect->getDepthStencilState();
	D3D11_DEPTH_STENCIL_DESC updateSStateDesc = {};
	updateSState->GetDesc(&updateSStateDesc);
	////setup appropriate DepthStencil states for rendering to the stencil buffer
	updateSStateDesc.DepthEnable = true;
	updateSStateDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	updateSStateDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	updateSStateDesc.StencilEnable = true;
	updateSStateDesc.StencilReadMask = 0xFF;
	updateSStateDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	// Keep original value on fail.
	updateSStateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	updateSStateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Increment to the stencil on pass.
	updateSStateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	updateSStateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// Stencil operations if pixel is back-facing.
	updateSStateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	updateSStateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	// Decrement the stencil on pass.
	updateSStateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_INCR;
	updateSStateDesc.BackFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	updateSState->Release();
	system->GetD3DDevice()->CreateDepthStencilState(&updateSStateDesc, &updateSState);

	planarShadowEffect->setDepthStencilState(updateSState);

	mainMenuInit = true;

};
void Menu::renderMainMenu(PlayerKart *playerKart, MenuState* MainMenuState, float stepSize)
{

	ID3D11DeviceContext* context = system->GetD3DDeviceContext();
	if (!mainMenuInit)
		initMainMenu();
	//Render Kart and Shadow
	ID3D11ShaderResourceView* mytexSRV = backGroundTex->getShaderResourceView();
	context->PSSetShaderResources(0, 1, &mytexSRV);
	context->PSSetShaderResources(1, 1, &mytexSRV);
	context->VSSetShader(screenQuadVS, 0, 0);
	context->PSSetShader(screenQuadPS, 0, 0);

	// Clear the screen
	static const FLOAT clearColor[4] = { 0.6f, 0.8f, 0.9f, 1.0f };
	context->ClearRenderTargetView(system->GetBackBufferRenderTargetView(), clearColor);
	context->ClearDepthStencilView(system->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	backGround->render(context);

	//Render planar shadow
	XMMATRIX  shadowM = DirectX::XMMatrixShadow(XMVectorSet(0, 1, 0, 0.9), XMVectorSet(0, 1, 0, 1));
	XMMATRIX shadowM2 = DirectX::XMMatrixShadow(XMVectorSet(0, 1, 0, 0.9), XMVectorSet(0, 0.9, 0, 1));
	float scale = 0.007;
	//draw kart shadow
	for (float i = 0; i < 10; i += 0.5)
	{
		shadowM = DirectX::XMMatrixShadow(XMVectorSet(0, 1, 0, 0.85), XMVectorSet(0, 1.0 - i * i * scale * 1.0, 0, 1));
		drawShadow(playerKart->getFrame(), planarShadowEffect, &shadowM, context);
		shadowM2 = DirectX::XMMatrixShadow(XMVectorSet(0, 1, 0, 0.85), XMVectorSet(0, 1.0 - i * i * scale, 0, 1));
		drawShadow(playerKart->getRearTires(), planarShadowEffect, &shadowM2, context);
		drawShadow(playerKart->getFLTire(), planarShadowEffect, &shadowM2, context);
		drawShadow(playerKart->getFRTire(), planarShadowEffect, &shadowM2, context);
		drawShadow(playerKart->getFarings(), planarShadowEffect, &shadowM2, context);
		context->ClearDepthStencilView(system->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}

	//Render Kart
	lookAtCamera->update(context);
	playerKart->render(context);

	//Render IMGUI HUD


	// Start the Dear ImGui frame
	#ifdef PC_BUILD
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	#else
	ImGui_ImplUwp_NewFrame();
	static int c = 0;
	if (c > 1000)
		*MainMenuState = MenuDone;
	c++;
	#endif

	ImGui::NewFrame();
	{
		ImGui::SetNextWindowBgAlpha(0.0);
		ImGui::Begin("Framerate", 0, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		ImGui::SetWindowFontScale(1.0);
		ImGui::SetWindowSize(ImVec2(1024, 1024));

		if (*MainMenuState == MainMenu)
		{
			ImGui::SetWindowPos(ImVec2(2, 2));

			if (ImGui::Button("Create Lobby"))
			{
				*MainMenuState = CreateLobby;
				netMgr->createLobby();
				netMgr->host = true;
			}

			if (ImGui::Button("Join Lobby"))
				*MainMenuState = JoinLobby;

			if (ImGui::Button("Play Offline"))
				*MainMenuState = MenuDone;
			
			bool LeftArrowButtonPressed =  ImGui::ArrowButton("##left", ImGuiDir_Left);
			if (LeftArrowButtonPressed)
			{
				playerKartWrap++;
				if (playerKartWrap >= kartTextures->size())
					playerKartWrap = 0;
				ID3D11ShaderResourceView* faringTexArray[] = { (*kartTextures)[playerKartWrap]->getShaderResourceView(),(*kartTextures)[playerKartWrap]->getShaderResourceView() };
				playerKart->getFarings()->instances[0].materials[0]->setTextures(faringTexArray, 2);
			};
			
			ImGui::SameLine(75.0f);
			string wrapNameTxt = string("Wrap: " + (*kartWraps)[playerKartWrap].substr(0, (*kartWraps)[playerKartWrap].find_last_of('.')));

			ImGui::Text(wrapNameTxt.c_str());
			ImGui::SameLine(400.0f);
			bool RightArrowButtonPressed =  ImGui::ArrowButton("##right", ImGuiDir_Right);
			if (RightArrowButtonPressed)
			{
				playerKartWrap--;
				if (playerKartWrap < 0)
					playerKartWrap = kartTextures->size() - 1;
				ID3D11ShaderResourceView* faringTexArray[] = { (*kartTextures)[playerKartWrap]->getShaderResourceView(), (*kartTextures)[playerKartWrap]->getShaderResourceView() };
				playerKart->getFarings()->instances[0].materials[0]->setTextures(faringTexArray, 2);
			};


			char* items[] = { "Lowest", "Low", "Medium", "High", "Highest" };
			static bool SelectingQuality = false;
			string tmp = "Quality - "; tmp.append(items[quality]);
			if (ImGui::Button(tmp.c_str()))
				SelectingQuality = true;

			if (SelectingQuality)
			{
				for (int n = 0; n < IM_ARRAYSIZE(items); n++)
				{
					const bool is_selected = (quality == n);
					if (ImGui::Button(items[n]))
					{
						SelectingQuality = false;
						quality = n;
					}
					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
			}
		}
		else if (*MainMenuState == CreateLobby)
		{
			ImGui::SetWindowPos(ImVec2(2, 2));
			ImGui::Text("%s - Waiting for players", netMgr->lobbys[0].name.c_str());
			ImGui::Text("Number of Players: %d", netMgr->lobbys[0].numPlayers);

			for (int i = 0; i < netMgr->lobbys[0].numPlayers; i++)
				ImGui::Text("Player%d: %s", i + 1, netMgr->lobbys[0].playerHandles[i].c_str());
			if (ImGui::Button("Start Game"))
			{
				// Set focus on Button B
				*MainMenuState = MenuDone;
				netMgr->startGame();
			}

		}
		else if (*MainMenuState == JoinLobby)
		{
			netMgr->host = false;
			ImGui::SetWindowPos(ImVec2(5, 5));
			netMgr->findLobbys();

			if (netMgr->num_lobbys > 0)
			{
				ImGui::Text("Select Lobby");
				ImGui::Text("Number of Lobbys: %d", netMgr->num_lobbys);
				static int lobbySelected = 0;

				string items2 = "";
				for (int i = 0; i < netMgr->num_lobbys; i++)
				{
					items2 += netMgr->lobbys[i].name;
					items2 += '\0';
				}

				ImGui::Combo("##0", &lobbySelected, items2.data(), netMgr->num_lobbys);

				for (int i = 0; i < netMgr->lobbys[lobbySelected].numPlayers; i++)
					ImGui::Text("Player%d: %s", i + 1, netMgr->lobbys[lobbySelected].playerHandles[i].c_str());

				if (ImGui::Button("Join"))
				{
					*MainMenuState = WaitingToStart;
					netMgr->joinLobby(lobbySelected);
				}
			}
			else
				ImGui::Text("Searching for Lobbys");
		}
		else if (*MainMenuState == WaitingToStart)
		{
			ImGui::SetWindowPos(ImVec2(2, 2));
			ImGui::Text("Waiting for host to start game (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate);
		}
		ImGui::End();
	}


	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present current frame to the screen
	system->Present();
}
void Menu::renderGUIMenu(MenuState* MainMenuState, Kart* kartArray[],int nKarts) {
	
	if (*MainMenuState != MenuNone)
	{

		//Render IMGUI HUD	ImGui_ImplDX11_NewFrame();

#ifdef PC_BUILD
		ImGui_ImplWin32_NewFrame();
		ImGui_ImplDX11_NewFrame();
#else
		ImGui_ImplUwp_NewFrame();
#endif

		ImGui::NewFrame();
		{
			ImGui::SetNextWindowBgAlpha(0.0);
			ImGui::Begin("Framerate", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
			ImGui::SetWindowFontScale(0.5);
			ImGui::SetWindowSize(ImVec2(600, 600));
			ImGui::SetWindowPos(ImVec2(2, 2));

			ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			ImGui::Text("RPM: %.3f", kartArray[0]->getVehicle4W()->mDriveDynData.getEngineRotationSpeed() * 10.0);
			ImGui::Text("Speed: %.3f", kartArray[0]->getVehicle4W()->computeForwardSpeed() * 2.23694);

			ImGui::Text("Lap Time: %.3f", kartArray[0]->getLapTime());
			if(kartArray[0]->getLastLapTime()>0)
				ImGui::Text("Last Lap Time: %.3f", kartArray[0]->getLastLapTime());
			else
				ImGui::Text("Last Lap Time: N/A");
			if(kartArray[0]->getBestLapTime()<1000)
			ImGui::Text("Best Lap Time: %.3f", kartArray[0]->getBestLapTime());
			else
				ImGui::Text("Best Lap Time: N/A", kartArray[0]->getBestLapTime());

			static int currentItem = 0;
			const char* items[] = { "150cc", "200cc", "270cc" };

			currentItem = kartArray[0]->getKartCC();
			ImGui::Text("Engine: %s", items[currentItem]);

			if (kartArray[0]->getDisc())
			{
				ImGui::SetNextItemWidth(60);
				ImGui::SetWindowFontScale(1.0);
				ImGui::Text("Disqualified");
				ImGui::SetWindowFontScale(0.5);
			}

			ImGui::Text("Leader Board");
			float bestTime = 1000;
			float priorBest = 0;
			int nextKart = -1;
			for (int j = 0; j < nKarts; j++)
			{
				for (int i = 0; i < nKarts; i++)
					if (kartArray[i]->getBestLapTime() < bestTime && kartArray[i]->getBestLapTime() > priorBest)
					{
						bestTime = kartArray[i]->getBestLapTime();
						nextKart = i;
					}
				if (nextKart > -1)
				{
					priorBest = bestTime;
					bestTime = 1000;
					ImGui::Text("Kart %d Best Lap Time: %.3f", nextKart, kartArray[nextKart]->getBestLapTime());
					nextKart = -1;
				}

			}


			ImGui::End();
		}
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	}
}



