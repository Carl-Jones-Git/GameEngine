#pragma once

#include "..\Common\DeviceResources.h"
#include "ShaderStructures.h"
#include "..\Common\StepTimer.h"
#include "..\Common\MoveLookController.h"
#include <Source\Box.h>
#include <Source\Camera.h>
#include <Source\FirstPersonCamera.h>
#include <Source\Texture.h>
#include <Source\Terrain.h>
#include "LightArray.h"
#include <Source\Model.h>
#include <Source\Flare.h>
#include <Source\ShadowMap.h>
#include <functional>
#include <Source/Model.h>
#include <Kart.h>
#include <AIKart.h>
#include <PlayerKart.h>
#include <PhysXVehicleController.h>
#include <PhysXKarting.h>

#include <Menu.h>

#include <imgui.h>
#include <imgui_impl_dx11.h>


#define MAX_DRAGONS 8
#define NUM_DRAGONS 0
enum PhysXMode { STATIC, DYNAMIC, DRIVEABLE };
#include <filesystem>
#include <TinyXML\Tinyxml2.h>
namespace ti = tinyxml2;
namespace fs = std::filesystem;

namespace App4
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Scene
	{
	public:
		Scene(const std::shared_ptr<DX::DeviceResources>& deviceResources, MoveLookController ^ controller);
		void CreateDeviceDependentResources();
		void CreateWindowSizeDependentResources();
		void ReleaseDeviceDependentResources();
		void Update(DX::StepTimer const& timer);
		void Render();
		std::wstring getTextBuffer() { return textBuffer; };
		void updateDragons(double gT[]);
		void UpdateDragonFire(ID3D11DeviceContext *context, DirectX::XMMATRIX dragonMat, float dragonHeight, double currentAnimTime);
		void RenderSceneObjects(ID3D11DeviceContext *context);


	private:
		Menu* menu = nullptr;
		MenuState MainMenuState = Intro;
		NetMgr netMgr;
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		MoveLookController^  m_controller;

		std::wstring textBuffer = L"loading";
		CBufferScene* cBufferSceneCPU = nullptr;
		ID3D11Buffer *cBufferSceneGPU = nullptr;
	
			//Sounds
		FMOD::System* FMSystem;

		//PhysX Attributes
		float acc = 0.0f;
		float stepSize = 1.0f / 60.0f;
		int numPXUpdates = 1;
		int numScenePXUpdates = 1;
		int numKartPXUpdates = 1;
		PhysXKarting* PXScene = nullptr;
		//Box* physXBox = nullptr;

		XMMATRIX* boxMatArray;
		//BlurUtility* bloom = nullptr;
		//Kart Attributes
		Kart* kartArray[NUM_VEHICLES];
		AIKart* aiKarts[NUM_AI_VEHICLES + 1];
		PlayerKart* playerKart = nullptr;
		float menuKartRot = -0.3f;
		vector <std::string> kartWraps;
		vector <shared_ptr<Texture>> kartTextures;
		PhysXVehicleController* PXKartController = nullptr;
		NavPoints* navPoints = nullptr;
		vector <shared_ptr<Model>> models;

		int		terrainSizeWH = 200;
		float	terrainScaleXZ = 1.0f;
		float	terrainScaleY = 2.5;// 0.3f;
		float		grassLength = 0.001666f / terrainScaleY;
		int			numGrassPasses = 5;//Set by Quality
		//Model* tree = nullptr;
		Terrain* grass = nullptr;





		//Flares
		static const int	numFlares = 6;
		Flare* flares;

		//float grassLength = 0.002f;
		//int numGrassPasses = 12;
			// Add Textures to the scene
		Texture* cubeDayTexture = nullptr;
		Box *skyBox = nullptr;
		//Grid *water = nullptr;
		Model *orb = nullptr;
		Model *castle = nullptr;
		//SkinnedModel *dragon[MAX_DRAGONS];
		ShadowMap *shadowMap = nullptr;
		//GPUParticles * fire = nullptr;
		CBufferModel *cBufferParticlesCPU = nullptr;
		ID3D11Buffer							*cBufferParticlesGPU = nullptr;

		FirstPersonCamera *camera = nullptr;
		LightArray *sceneLights = nullptr;

		void DrawGrass(ID3D11DeviceContext *context);
		void DrawFlare(ID3D11DeviceContext* context);
		void renderShadowObjects(ID3D11DeviceContext* context);
		void load(string path, shared_ptr<Effect> effect, float mapScale = 2.0f, float LHCoords = -1.0f);
		void loadModel(ti::XMLElement* model, vector< shared_ptr<Model>>* modelList, shared_ptr<Effect>  effect, float mapScale = 1.0f, float LHCoords = -1.0f);
		void toPhysX(shared_ptr<Model> model, XMVECTOR R, XMVECTOR T, PhysXMode type = STATIC, int instance = 0);
		void updateSceneOrMenu();
		// Variables used with the rendering loop.
		bool	m_loadingComplete;
		bool paused;
		float	m_degreesPerSecond;

	};
}

