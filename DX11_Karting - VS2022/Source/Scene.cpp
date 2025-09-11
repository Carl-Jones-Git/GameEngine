// Scene.cpp

// Master include file - common practice in large projects
#include <Includes.h>
#include <string.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// System component includes
#include <System.h>
#include <CGDClock.h>
#include <Scene.h>
#include <Effect.h>
#include <VertexStructures.h>
#include <Texture.h>
#include <ShadowMap.h>
#include <Menu.h>

// Third-party library integrations
#include <imgui.h>              // GUI library for debugging/UI
#include <imgui_impl_dx11.h>    // DX11 implementation for ImGui
#include <PhysXVehicleController.h>
#include <PhysXVehicleUtils.h>

// Resource path configuration based on input method
#ifdef PC_BUILD
wstring ResourcePath = L"..\\";
string ShaderPath = "Shaders\\cso\\";
#else
wstring ResourcePath = L"Assets\\";
string ShaderPath = "";
#endif 

// Global variables for skin rendering parameters
float gUseTSD = 1.0f;
float gLightDistance = 10.0f;
float gLightVec[3] = { 0.0f, 10.0f, 10.0f };
float gRed = 1.0f;

// External PhysX variables (shared across modules)
extern bool gVehicleOrderComplete;
extern PxScene* mScene;
extern PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs;
extern ShapeUserData gShapeUserDatas;
extern physx::PxRigidDynamic* body[];
extern PxPhysics* mPhysics;
extern PxCooking* gCooking;
extern PxMaterial* mMaterial;
extern PxMaterial* grassMaterial;
extern PxRigidStatic* gDrivableGroundPlane;

// Constants for PhysX box setup
const float boxHalfSize = 0.5;
const physx::PxU32 boxSize = 5;
const int S = (boxSize * (boxSize + 1.0)) / 2;

// Start/finish position for race track
XMVECTOR startFinishPos = XMVectorSet(36, 0.0f, 0.7f, 1.0f);
float startDist = 10.0f;
float finishDist = 10.0f;

float terrainScale = 2.0;
bool renderFrame = false;  // Control flag for rendering

// Convert model to PhysX shape - demonstrates physics/rendering integration
void Scene::toPhysX(shared_ptr<Model> model, XMVECTOR R, XMVECTOR T, PhysXMode type, int instance)
{
	PxMaterial* material = mPhysics->createMaterial(0.0f, 0.0f, 0.6f);
	if (type == DRIVEABLE)
		material = mPhysics->createMaterial(0.6f, 0.4f, 0.6f);

	// Manual memory management with malloc/free
	PxVec3* verts = (PxVec3*)malloc(sizeof(PxVec3) * model->getNumVert());
	PxTransform t(PxVec3(0, 0, 0), PxQuat(XMConvertToRadians(0), PxVec3(0, 1, 0)));

	// Transform vertices from model to world space
	for (int i = 0; i < model->getNumVert(); i++)
	{
		XMVECTOR pos = DirectX::XMVector3TransformCoord(
			XMVectorSet(model->getVertexBufferCPU()[i].pos.x,
				model->getVertexBufferCPU()[i].pos.y,
				model->getVertexBufferCPU()[i].pos.z, 1),
			model->getWorldMatrix());
		verts[i].x = DirectX::XMVectorGetX(pos);
		verts[i].y = DirectX::XMVectorGetY(pos) - 0.25;
		verts[i].z = DirectX::XMVectorGetZ(pos);
	}

	// Process indices for physics mesh
	int count = 0;
	PxU32* indices32 = (PxU32*)malloc(sizeof(PxU32) * model->getNumInd());
	for (int j = 0; j < model->getNumMeshes(); j++)
		for (int i = 0; i < model->getIndexCount()[j]; i++) {
			indices32[count] = (PxU32)model->getIndexBufferCPU()[count] + model->getBaseVertexOffset()[j];
			count++;
		}

	// Create PhysX triangle mesh for static objects
	if (type == STATIC || type == DRIVEABLE)
	{
		PxTriangleMesh* triMesh = createTriangleMesh(verts, model->getNumVert(),
			indices32, model->getNumFaces(),
			*mPhysics, *gCooking);
		PxRigidStatic* r = mPhysics->createRigidStatic(t);
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*r, PxTriangleMeshGeometry(triMesh), *material);

		// Setup collision filters for game objects
		PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_CHASSIS, 0, 0);
		shape->setSimulationFilterData(simFilterData);

		if (type == DRIVEABLE)
		{
			PxFilterData qryFilterData;
			setupDrivableSurface(qryFilterData);
			shape->setQueryFilterData(qryFilterData);
		}

		mScene->addActor(*r);
		free(verts);
		free(indices32);
	}
	else // Dynamic objects use convex meshes
	{
		PxTransform t(PxVec3(DirectX::XMVectorGetX(T), DirectX::XMVectorGetY(T) + 0.09,
			DirectX::XMVectorGetZ(T)),
			PxQuat(DirectX::XMVectorGetX(R), DirectX::XMVectorGetY(R),
				DirectX::XMVectorGetZ(R), DirectX::XMVectorGetW(R)));

		// Create PhysX convex mesh for dynamic objects
		PxConvexMesh* convMesh = createConvexMesh(verts, model->getNumVert(), *mPhysics, *gCooking);
		PxRigidDynamic* r = mPhysics->createRigidDynamic(t);
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*r, PxConvexMeshGeometry(convMesh), *material);

		// Setup collision filters
		PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_GROUND |
			COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE, 0, 0);
		shape->setSimulationFilterData(simFilterData);

		// Store reference to physics object in model instance
		model->instances[instance].dynamicPX = r;
		mScene->addActor(*r);
		free(verts);
	}
}

// Load models from XML - demonstrates external asset configuration
void Scene::load(string path, shared_ptr<Effect> effect, float mapScale, float LHCoords)
{
	models.clear();
	ti::XMLDocument doc;
	doc.LoadFile(path.c_str());
	if (doc.Error())
		std::cerr << "Error parsing XML: " << doc.ErrorStr() << std::endl;
	else
	{
		ti::XMLElement* scene = doc.RootElement();
		ti::XMLElement* model = scene->FirstChildElement("Model");
		models.clear();

		if (model) // Load models if available
			loadModel(model, &models, effect);

		// Special handling for specific model types
		if (models.size() > 1)
		{
			// Model[0] is assumed to be the Racing Line
			models[0]->getMaterial(0)->setEmissive(XMFLOAT4(1, 1, 1, 1));

			// Custom sampler for racing line arrows
			D3D11_SAMPLER_DESC linearDesc;
			ZeroMemory(&linearDesc, sizeof(D3D11_SAMPLER_DESC));
			linearDesc.Filter = D3D11_FILTER_ANISOTROPIC;
			linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
			linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
			linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
			linearDesc.MaxAnisotropy = 8;
			linearDesc.MipLODBias = 0.0f;
			linearDesc.MinLOD = 0.0f;
			linearDesc.MaxLOD = 0.0f;
			linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;

			ID3D11SamplerState* sampler;
			system->getDevice()->CreateSamplerState(&linearDesc, &sampler);
			models[0]->setSampler(sampler);

			// Model[1] is assumed to be the boundary fence
			shared_ptr<Effect> effectA2C = make_shared<Effect>(effect);
			effectA2C->setCullMode(system->getDevice(), D3D11_CULL_NONE);
			effectA2C->setAlphaToCoverage(system->getDevice(), TRUE);
			models[1]->setEffect(effectA2C);
		}
	}
}

// Load individual model from XML element
void Scene::loadModel(ti::XMLElement* model, vector<shared_ptr<Model>>* modelList,
	shared_ptr<Effect> effect, float mapScale, float LHCoords)
{
	ID3D11DeviceContext* context = system->getDeviceContext();
	ID3D11Device* device = system->getDevice();

	while (model)
	{
		string path = model->FirstChildElement("PathName")->GetText();
		string data = model->FirstChildElement("Data")->GetText();
		fs::path pathName(path);
		shared_ptr<Material> mat(new Material(device));

		// Load position, rotation and scale from XML
		XMVECTOR pos = vecFromStr(data);
		pos = DirectX::XMVectorSetX(pos, DirectX::XMVectorGetX(pos) * mapScale);
		pos = DirectX::XMVectorSetZ(pos, DirectX::XMVectorGetZ(pos) * mapScale);
		pos = DirectX::XMVectorSetY(pos, grass->CalculateYValueWorld(
			DirectX::XMVectorGetX(pos), DirectX::XMVectorGetZ(pos)) + DirectX::XMVectorGetY(pos));

		XMVECTOR rot = vecFromStr(data);
		XMVECTOR scale = vecFromStr(data);
		scale = DirectX::XMVectorScale(scale, mapScale);
		XMMATRIX rotM = XMMatrixRotationX(DirectX::XMVectorGetX(rot)) *
			XMMatrixRotationY(DirectX::XMVectorGetY(rot)) *
			XMMatrixRotationZ(DirectX::XMVectorGetZ(rot));

		// Get PhysX mode from XML
		ti::XMLElement* physXEl = model->FirstChildElement("PhysX");
		string physX = "No";
		if (physXEl) physX = physXEl->GetText();

		if (physX.compare("Dynamic") == 0)
		{
			// Dynamic models cannot be scaled in PhysX, so pre-transform vertices
			XMMATRIX PreTransScaleMat = XMMatrixScaling(DirectX::XMVectorGetX(scale),
				DirectX::XMVectorGetY(scale),
				DirectX::XMVectorGetZ(scale));

			shared_ptr<Model> mod(new Model(context, device,
				wstring(pathName.wstring().substr(3, path.length())), effect, nullptr, -1, &PreTransScaleMat));

			modelList->push_back(mod);
			(*modelList)[modelList->size() - 1]->setWorldMatrix(
				rotM * XMMatrixTranslation(DirectX::XMVectorGetX(pos),
					DirectX::XMVectorGetY(pos),
					DirectX::XMVectorGetZ(pos)));

			(*modelList)[modelList->size() - 1]->update(context);
			XMVECTOR Q = XMQuaternionRotationMatrix(rotM);

			// Create dynamic PhysX model
			toPhysX((*modelList)[modelList->size() - 1], Q, pos, DYNAMIC);
		}
		else
		{
			shared_ptr<Model> mod(new Model(context, device,
				wstring(pathName.wstring().substr(3, path.length())), effect, nullptr));

			modelList->push_back(mod);
			(*modelList)[modelList->size() - 1]->setWorldMatrix(
				rotM * XMMatrixScaling(DirectX::XMVectorGetX(scale),
					DirectX::XMVectorGetY(scale),
					DirectX::XMVectorGetZ(scale)) *
				XMMatrixTranslation(DirectX::XMVectorGetX(pos),
					DirectX::XMVectorGetY(pos),
					DirectX::XMVectorGetZ(pos)));

			(*modelList)[modelList->size() - 1]->update(context);

			if (physX.compare("Static") == 0)
				toPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), STATIC);
			else if (physX.compare("Driveable") == 0)
				toPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), DRIVEABLE);
		}

		cout << "Loaded:" << pathName.string() << endl;

		// Load instances if available
		ti::XMLElement* modelInstance = model->FirstChildElement("Instance");
		while (modelInstance)
		{
			data = modelInstance->FirstChildElement("Data")->GetText();
			XMVECTOR posI = vecFromStr(data);
			posI = DirectX::XMVectorSetX(posI, DirectX::XMVectorGetX(posI) * mapScale);
			posI = DirectX::XMVectorSetZ(posI, DirectX::XMVectorGetZ(posI) * mapScale);
			posI = DirectX::XMVectorSetY(posI, grass->CalculateYValueWorld(
				DirectX::XMVectorGetX(posI), DirectX::XMVectorGetZ(posI)) + DirectX::XMVectorGetY(posI));

			XMVECTOR rotI = vecFromStr(data);
			XMVECTOR scaleI = vecFromStr(data);
			scaleI = DirectX::XMVectorScale(scaleI, mapScale);
			XMMATRIX rotMI = XMMatrixRotationX(DirectX::XMVectorGetX(rotI)) *
				XMMatrixRotationY(DirectX::XMVectorGetY(rotI)) *
				XMMatrixRotationZ(DirectX::XMVectorGetZ(rotI));

			// Currently, PhysX properties of instances are inherited from parent
			if (physX.compare("Dynamic") == 0)
			{
				XMVECTOR QI = XMQuaternionRotationMatrix(rotMI);
				(*modelList)[modelList->size() - 1]->instances.push_back(
					Instance(rotMI * XMMatrixTranslation(DirectX::XMVectorGetX(posI),
						DirectX::XMVectorGetY(posI),
						DirectX::XMVectorGetZ(posI)),
						(*modelList)[modelList->size() - 1]->getMaterials(0)));

				toPhysX((*modelList)[modelList->size() - 1], QI, posI, DYNAMIC,
					(*modelList)[modelList->size() - 1]->instances.size() - 1);
			}
			else
			{
				(*modelList)[modelList->size() - 1]->instances.push_back(
					Instance(rotMI * XMMatrixScaling(DirectX::XMVectorGetX(scaleI),
						DirectX::XMVectorGetY(scaleI),
						DirectX::XMVectorGetZ(scaleI)) *
						XMMatrixTranslation(DirectX::XMVectorGetX(posI),
							DirectX::XMVectorGetY(posI),
							DirectX::XMVectorGetZ(posI)),
						(*modelList)[modelList->size() - 1]->getMaterials(0)));

				if (physX.compare("Static") == 0)
					toPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), STATIC);
				else if (physX.compare("Driveable") == 0)
					toPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), DRIVEABLE);
			}
			modelInstance = modelInstance->NextSiblingElement("Instance");
		}
		model = model->NextSiblingElement("Model");
	}
}

// Viewport setup for DirectX
HRESULT Scene::rebuildViewport() {
	ID3D11DeviceContext* context = system->getDeviceContext();
	if (!context)
		return E_FAIL;

	// Bind render target and depth/stencil views
	ID3D11RenderTargetView* renderTargetView = system->getBackBufferRTV();
	context->OMSetRenderTargets(1, &renderTargetView, system->getDepthStencil());

	// Setup viewport for main window
	RECT clientRect;
	GetClientRect(wndHandle, &clientRect);
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<FLOAT>(clientRect.right - clientRect.left);
	viewport.Height = static_cast<FLOAT>(clientRect.bottom - clientRect.top);
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	// Set Viewport
	context->RSSetViewports(1, &viewport);

	return S_OK;
}

// Main resource setup for the application
HRESULT Scene::initialiseSceneResources() {
	ID3D11DeviceContext* context = system->getDeviceContext();
	ID3D11Device* device = system->getDevice();
	if (!device)
		return E_FAIL;

	// Set up viewport for the main window
	rebuildViewport();

	// Draw intro screen while loading
	if (MainMenuState == Intro)
	{
		menu = new Menu(system);
		menu->init(&netMgr);
		menu->renderIntro();
	}

	// Initialize FMOD audio system
	FMOD_RESULT result = FMOD::System_Create(&FMSystem);
	if (result != FMOD_OK)
	{
		FMSystem->release();
		delete FMSystem;
		FMSystem = nullptr;
		cout << "cant create sound system" << endl;
	}

	result = FMSystem->init(32, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)
	{
		FMSystem->release();
		delete FMSystem;
		FMSystem = nullptr;
		cout << "cant init sound" << endl;
	}

	// Initialize PhysX
	PXScene = new PhysXKarting();

	// Create shadow map
	XMVECTOR lightVec = XMVectorSet(-124.0, 50, 60, 1.0);
	shadowMap = new ShadowMap(device, lightVec, 4096);

	// Initialize post-processing effects
	bloom = new BlurUtility(system->getDevice(), context, 300, 240);
	skinSim = new SkinUtility(system->getDevice(), context, 1068, 712);

	// Setup main rendering effects
	shared_ptr<Effect> perPixelLightingEffect(new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso",
		"Shaders\\cso\\per_pixel_lighting_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));

	shared_ptr<Effect> reflectionMappingEffect(new Effect(device, "Shaders\\cso\\reflection_map_vs.cso",
		"Shaders\\cso\\reflection_map_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));

	// SkyBox effect with specialized render states
	shared_ptr<Effect> skyBoxEffect(new Effect(device, "Shaders\\cso\\sky_box_vs.cso",
		"Shaders\\cso\\sky_box_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));
	skyBoxEffect->setCullMode(device, D3D11_CULL_FRONT);
	skyBoxEffect->setDepthFunction(device, D3D11_COMPARISON_LESS_EQUAL);

	// Skin rendering effect
	skinEffect = make_shared<Effect>(device, "Shaders\\cso\\per_pixel_lighting_vs.cso",
		"Shaders\\cso\\basic_skin_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc));

	// Face texture setup
	Texture* faceTexture = new Texture(device, L"..\\Resources\\Textures\\Face\\IMG_9158ds.png");
	Texture* faceSpecTexture = new Texture(device, L"..\\Resources\\Textures\\Face\\IMG_9158s.png");
	Texture* faceNormTexture = new Texture(device, L"..\\Resources\\Textures\\Face\\IMG_9158ns15f9ix.png");

	ID3D11ShaderResourceView* faceTextureArray[] = { faceTexture->getShaderResourceView(),
													faceNormTexture->getShaderResourceView(),
													faceSpecTexture->getShaderResourceView() };

	shared_ptr<Material> skinMat(new Material(device));
	skinMat->setSpecular(XMFLOAT4(1.0, 1.0, 1.0, 1.0));
	skinMat->setTextures(faceTextureArray, 3);
	skinEffect->setCullMode(device, D3D11_CULL_NONE);

	face = new Model(context, device, wstring(L"..\\Resources\\Models\\Face\\Peter2.3ds"), skinEffect, skinMat);
	face->setWorldMatrix(XMMatrixScaling(0.03, 0.03, 0.03) *
		XMMatrixRotationZ(XMConvertToRadians(-2)) *
		XMMatrixTranslation(-0.3, 5.2, 1));
	face->update(context);

	// Setup cube environment texture
	cubeDayTexture = new Texture(device, L"..\\Resources\\Textures\\grassenvmap1024.dds");
	ID3D11ShaderResourceView* cubeDayTextureSRV = cubeDayTexture->getShaderResourceView();

	// Bind cube texture to shader slot 6 (used by multiple effects)
	context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);

	// Create skybox
	shared_ptr<Material> skyBoxMaterial(new Material(device));
	skyBoxMaterial->setTexture(Texture(device, L"..//Resources\\Textures\\nightenvmap1024.dds").getShaderResourceView());
	skyBox = new Box(device, skyBoxEffect, skyBoxMaterial);
	skyBox->setWorldMatrix(skyBox->getWorldMatrix() * XMMatrixScaling(1000, 1000, 1000));
	skyBox->update(context);

	// Setup water rendering
	dynamicCubeMap = new DynamicCube(device, context, XMVectorSet(-54.0, 1.0, 66.0, 1), 512);
	dynamicCubeMap->updateCubeCameras(context, XMVectorSet(-54.0, 0.0, 66.0, 1));
	ID3D11ShaderResourceView* dynamicCubeMapSRV = dynamicCubeMap->getSRV();
	context->PSSetShaderResources(6, 1, &dynamicCubeMapSRV);

	shared_ptr<Material> waterMaterial(new Material(device));
	waterMaterial->setTexture(Texture(device, L"..\\Resources\\Textures\\Waves.dds").getShaderResourceView());

	shared_ptr<Effect> waterEffect(new Effect(device, "Shaders\\cso\\ocean_vs.cso",
		"Shaders\\cso\\ocean_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));

	water = new Grid(10, 10, device, waterEffect, waterMaterial);
	water->setWorldMatrix(XMMatrixScaling(3.0, 0.3, 3.0) *
		XMMatrixTranslation(-68.0, 0.9, 54.0));
	water->update(context);

	// Create box material for PhysX boxes
	shared_ptr<Material> boxMat(new Material(device, XMFLOAT4(-1.0, -1.0, 1.0, 1.0)));
	boxMat->setSpecular(XMFLOAT4(0.3, 0.3, 0.3, 0.01));
	boxMat->setUsage(DIFFUSE_MAP);
	boxMat->setTexture(Texture(device, L"..\\Resources\\Textures\\WoodCrate02.dds").getShaderResourceView());

	physXBox = new Box(device, perPixelLightingEffect, boxMat);
	boxMatArray = new XMMATRIX[S];

	// Foliage and terrain setup
	shared_ptr<Effect> grassEffect(new Effect(device, "Shaders\\cso\\grass_vs.cso",
		"Shaders\\cso\\grass_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));

	// Alpha blending for grass
	grassEffect->setAlphaBlendEnable(device, TRUE);

	// Load grass textures
	Texture grassAlpha = Texture(device, L"..\\Resources\\Textures\\grassAlpha.tif");
	Texture grassDiffuse = Texture(device, L"..\\Resources\\Textures\\BrightonKarting5.bmp");
	Texture grassColour = Texture(device, L"..\\Resources\\Textures\\grass_texture.dds");
	Texture groundNormals = Texture(device, L"..\\Resources\\Textures\\Seamless_Asphalt_Texture_NORMAL.jpg");
	Texture asphaltDiffuse = Texture(device, L"..\\Resources\\Textures\\asphalt.dds");
	Texture heightMap = Texture(device, L"..\\Resources\\Levels\\Terrain01.bmp");
	Texture noiseMap = Texture(device, L"..\\Resources\\Levels\\noise.bmp");

	ID3D11ShaderResourceView* noise = noiseMap.getShaderResourceView();
	context->PSSetShaderResources(8, 1, &noise);
	context->VSSetShaderResources(8, 1, &noise);

	ID3D11ShaderResourceView* grassTextureArray[] = {
		grassDiffuse.getShaderResourceView(),
		asphaltDiffuse.getShaderResourceView(),
		grassAlpha.getShaderResourceView(),
		grassColour.getShaderResourceView(),
		groundNormals.getShaderResourceView(),
		noiseMap.getShaderResourceView()
	};

	shared_ptr<Material> grassMaterial(new Material(device));
	grassMaterial->setTextures(grassTextureArray, 6);

	// Create grassy terrain
	float terrainOffsetXZ = -(terrainSizeWH * terrainScaleXZ) / 2.0f;
	grass = new Terrain(device, context, terrainSizeWH, terrainSizeWH,
		heightMap.getTexture(), grassEffect, grassMaterial);

	grass->setWorldMatrix(XMMatrixScaling(terrainScaleXZ, terrainScaleY, terrainScaleXZ) *
		XMMatrixTranslation(terrainOffsetXZ, -0.085, terrainOffsetXZ));
	grass->update(context);
	grass->setColourMap(device, context, grassDiffuse.getTexture());

	// Load AI Kart navigation points
	navPoints = new NavPoints();
	navPoints->load("..\\Resources\\Levels\\NavSet01.xml");

	// Load orb for rendering NavPoints
	shared_ptr<Material> beeMat(new Material(device));
	shared_ptr<Material> redMat(new Material(device, XMFLOAT4(1.0, 0.0, 0.0, 1.0)));
	redMat->setDiffuse(XMFLOAT4(1.0, 0.0, 0.0, 1.0));
	redMat->setSpecular(XMFLOAT4(1.0, 0.0, 0.0, 1.0));

	beeMat->setTexture(Texture(device, L"..\\Resources\\Textures\\orb.tif").getShaderResourceView());
	beeMat->setUsage(EMISSIVE_MAP);

	shared_ptr<Effect> beeEffect(new Effect(device, "Shaders\\cso\\per_pixel_lighting_vs.cso",
		"Shaders\\cso\\emissive_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));

	orb3 = new Model(context, device, wstring(L"..\\Resources\\Models\\sphere.obj"), beeEffect, beeMat, 0);

	XMMATRIX beeInstanceMat = XMMatrixScaling(0.025, 0.025, 0.025) *
		XMMatrixTranslation(4.13, 0, 58.30);

	orb3->instances.push_back(Instance(beeInstanceMat, beeMat));

	XMMATRIX orbInstanceMat = XMMatrixScaling(0.5, 0.5, 0.5) *
		XMMatrixTranslation(0, grass->CalculateYValueWorld(0, 0) + 5.0, 0);

	orb3->instances.push_back(Instance(orbInstanceMat, redMat));

	// Skinning effect for animated models
	shared_ptr<Effect> skinningEffect(new Effect(device, "Shaders\\cso\\skinning_vs.cso",
		"Shaders\\cso\\per_pixel_lighting_ps.cso",
		skinVertexDesc, ARRAYSIZE(skinVertexDesc)));

	// Load dragon model with animations
	Texture dragonNormalTexture = Texture(device, L"..\\Resources\\Models\\Black Dragon NEW\\textures\\Dragon_Nor.jpg");
	Texture dragonTexture = Texture(device, L"..\\Resources\\Models\\Black Dragon NEW\\textures\\Dragon_ground_color.jpg");

	ID3D11ShaderResourceView* dragonTextureArray[] = {
		dragonTexture.getShaderResourceView(),
		dragonNormalTexture.getShaderResourceView()
	};

	shared_ptr<Material> dragonMat(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
	dragonMat->setSpecular(XMFLOAT4(0.3, 0.3, 0.3, 0.01));
	dragonMat->setUsage(DIFFUSE_MAP | NORMAL_MAP);
	dragonMat->setTextures(dragonTextureArray, 2);

	dragon = new SkinnedModel(context, device,
		wstring(L"..\\Resources\\Models\\Black Dragon NEW\\Dragon_Baked_Actions_fbx_7.4_binary.fbx"),
		skinningEffect, dragonMat);

	dragon->loadBones(device);
	dragon->setWorldMatrix(dragon->getWorldMatrix() *
		XMMatrixScaling(0.001, 0.001, 0.001) *
		XMMatrixTranslation(10, 0, 0) *
		XMMatrixTranslation(-20, 0 + grass->CalculateYValueWorld(-20, -30), -30));

	dragon->setCurrentAnim(3);
	dragon->update(context);

	// Load Nathan character model
	Texture nathanTextureBlack = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_dif_black.jpg");
	Texture nathanTextureRed = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_dif_red.jpg");
	Texture nathanTextureBlue = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_dif_blue.jpg");
	Texture nathanNormalTexture = Texture(device, L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\tex\\rp_nathan_animated_003_norm_small.dds");

	ID3D11ShaderResourceView* nathanTextureArrayBlack[] = {
		nathanTextureBlack.getShaderResourceView(),
		nathanNormalTexture.getShaderResourceView()
	};

	ID3D11ShaderResourceView* nathanTextureArrayRed[] = {
		nathanTextureRed.getShaderResourceView(),
		nathanNormalTexture.getShaderResourceView()
	};

	ID3D11ShaderResourceView* nathanTextureArrayBlue[] = {
		nathanTextureBlue.getShaderResourceView(),
		nathanNormalTexture.getShaderResourceView()
	};

	shared_ptr<Material> nathanBlackMat(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
	nathanBlackMat->setSpecular(XMFLOAT4(0.1, 0.1, 0.1, 0.001));
	nathanBlackMat->setUsage(DIFFUSE_MAP | NORMAL_MAP);

	shared_ptr<Material> nathanRedMat(new Material(device, nathanBlackMat));
	nathanRedMat->setTextures(nathanTextureArrayRed, 2);

	shared_ptr<Material> nathanBlueMat(new Material(device, nathanBlackMat));
	nathanBlueMat->setTextures(nathanTextureArrayBlue, 2);

	nathanBlackMat->setTextures(nathanTextureArrayBlack, 2);

	nathan = new SkinnedModel(context, device,
		wstring(L"..\\Resources\\Models\\55-rp_nathan_animated_003_walking_fbx\\rp_nathan_animated_003_walking2.fbx"),
		skinningEffect, nathanBlackMat);

	nathan->loadBones(device);

	nathan->instances.push_back(Instance(nathan->getWorldMatrix() *
		XMMatrixScaling(0.022, 0.022, 0.022) *
		XMMatrixRotationY(XMConvertToRadians(180)) *
		XMMatrixTranslation(8.635422, 0 + grass->CalculateYValueWorld(8.635422, 53.300213), 53.300213),
		nathanBlueMat));

	nathan->instances.push_back(Instance(nathan->getWorldMatrix() *
		XMMatrixScaling(0.022, 0.022, 0.022) *
		XMMatrixRotationY(XMConvertToRadians(180)) *
		XMMatrixTranslation(9.635422, 0 + grass->CalculateYValueWorld(9.635422, 53.300213), 53.300213),
		nathanRedMat));

	nathan->setWorldMatrix(nathan->getWorldMatrix() *
		XMMatrixScaling(0.022, 0.022, 0.022) *
		XMMatrixTranslation(5, 0, 0) *
		XMMatrixTranslation(0, grass->CalculateYValueWorld(0, 0), 0));

	nathan->setCurrentAnim(0);
	nathan->update(context);

	// Load Sophia character model
	Texture sophiaTextureYellow = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_dif_yellow.jpg");
	Texture sophiaTextureWhite = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_dif.jpg");
	Texture sophiaTexturePink = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_dif_pink.jpg");
	Texture sophiaNormalTexture = Texture(device, L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\tex\\rp_sophia_animated_003_norm_small.dds");

	ID3D11ShaderResourceView* sophiaTextureArrayYellow[] = {
		sophiaTextureYellow.getShaderResourceView(),
		sophiaNormalTexture.getShaderResourceView()
	};

	ID3D11ShaderResourceView* sophiaTextureArrayWhite[] = {
		sophiaTextureWhite.getShaderResourceView(),
		sophiaNormalTexture.getShaderResourceView()
	};

	ID3D11ShaderResourceView* sophiaTextureArrayPink[] = {
		sophiaTexturePink.getShaderResourceView(),
		sophiaNormalTexture.getShaderResourceView()
	};

	shared_ptr<Material> sophiaMatYellow(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
	sophiaMatYellow->setSpecular(XMFLOAT4(0.1, 0.1, 0.1, 0.001));
	sophiaMatYellow->setUsage(DIFFUSE_MAP | NORMAL_MAP);
	sophiaMatYellow->setTextures(sophiaTextureArrayYellow, 2);

	shared_ptr<Material> sophiaMatWhite(new Material(device, sophiaMatYellow));
	sophiaMatWhite->setTextures(sophiaTextureArrayWhite, 2);

	shared_ptr<Material> sophiaMatPink(new Material(device, sophiaMatYellow));
	sophiaMatPink->setTextures(sophiaTextureArrayPink, 2);

	sophia = new SkinnedModel(context, device,
		wstring(L"..\\Resources\\Models\\35-rp_sophia_animated_003_idling_fbx\\rp_sophia_animated_003_idling.fbx"),
		skinningEffect, sophiaMatYellow);

	sophia->loadBones(device);

	sophia->instances.push_back(Instance(sophia->getWorldMatrix() *
		XMMatrixRotationX(XMConvertToRadians(-90)) *
		XMMatrixRotationY(XMConvertToRadians(180)) *
		XMMatrixScaling(0.009, 0.009, 0.009) *
		XMMatrixTranslation(8.135422, 0 + grass->CalculateYValueWorld(8.135422, 53.300213), 53.300213),
		sophiaMatWhite));

	sophia->instances.push_back(Instance(sophia->getWorldMatrix() *
		XMMatrixRotationX(XMConvertToRadians(-90)) *
		XMMatrixRotationY(XMConvertToRadians(180)) *
		XMMatrixScaling(0.009, 0.009, 0.009) *
		XMMatrixTranslation(9.135422, 0 + grass->CalculateYValueWorld(9.135422, 53.300213), 53.300213),
		sophiaMatPink));

	sophia->setWorldMatrix(sophia->getWorldMatrix() *
		XMMatrixRotationX(XMConvertToRadians(-90)) *
		XMMatrixRotationY(XMConvertToRadians(180)) *
		XMMatrixScaling(0.009, 0.009, 0.009) *
		XMMatrixTranslation(10.135422, 0 + grass->CalculateYValueWorld(10.135422, 53.300213), 53.300213));

	sophia->setCurrentAnim(0);
	sophia->update(context);

	// Generate random trees for foliage
	shared_ptr<Effect> treeEffect(new Effect(device, "Shaders\\cso\\tree_vs.cso",
		"Shaders\\cso\\tree_ps.cso",
		extVertexDesc, ARRAYSIZE(extVertexDesc)));

	treeEffect->setCullMode(device, D3D11_CULL_NONE);
	treeEffect->setAlphaToCoverage(device, TRUE);  // Alpha to coverage for foliage

	// Load tree textures
	Texture treeDiffuse = Texture(device, L"..\\Resources\\Textures\\tree.tif");
	shared_ptr<Material> treeMat(new Material(device, XMFLOAT4(0.3, 0, 0, 1.0)));
	treeMat->setSpecular(XMFLOAT4(0.0, 0.0, 0.0, 0.001));
	treeMat->setTexture(treeDiffuse.getShaderResourceView());

	// Load tree model
	tree = new Model(context, device, wstring(L"..\\Resources\\Models\\tree.3ds"), treeEffect, treeMat);
	tree->setWorldMatrix(XMMatrixTranslation(14.0f, grass->CalculateYValueWorld(13.0f, 20.0f), 20.0f));
	tree->update(context);

	// Create random tree instances
	for (int i = 0; i < numTrees; i++)
	{
		float red = ((float)rand() / RAND_MAX) * 0.5;
		float green = (((float)rand() / RAND_MAX) * 0.5) + 0.5;
		float x = (((float)rand() / RAND_MAX) + 0.4) * 50.0f;
		float z = (((float)rand() / RAND_MAX) + 0.4) * 25.0f;

		if (((float)rand() / RAND_MAX) > 0.5) x = -x;
		if (((float)rand() / RAND_MAX) > 0.5) z = -z;

		// Ensure trees are placed on valid terrain
		while (grass->getMapColour(x, z).x <= 0.0f)
		{
			x = (((float)rand() / RAND_MAX) + 0.4) * 50.0f;
			z = (((float)rand() / RAND_MAX) + 0.4) * 25.0f;
			if (((float)rand() / RAND_MAX) > 0.5) x = -x;
			if (((float)rand() / RAND_MAX) > 0.5) z = -z;
		}

		float s = (((float)rand() / RAND_MAX) + 0.5);
		float r = ((float)rand() / RAND_MAX);

		shared_ptr<Material> treeInstanceMat(new Material(device, XMFLOAT4(red, 0, 0, 1.0)));
		treeInstanceMat->setSpecular(XMFLOAT4(0.0, 0.0, 0.0, 0.001));
		treeInstanceMat->setTexture(treeDiffuse.getShaderResourceView());

		tree->instances.push_back(Instance(DirectX::XMMatrixRotationY(r) *
			XMMatrixScaling(s, s, s) *
			XMMatrixTranslation(x, grass->CalculateYValueWorld(x, z), z),
			treeInstanceMat));
	}

	// Setup particle systems for kart effects
	shared_ptr<Material> dirtMat(new Material(device));
	Texture grassSkid = Texture(device, L"..\\Resources\\Textures\\grass_texture2.jpg");
	dirtMat->setTexture(grassSkid.getShaderResourceView());

	shared_ptr<Effect> dirtEffect(new Effect(device, "Shaders\\cso\\fire_vs.cso",
		"Shaders\\cso\\dirt_ps.cso",
		particleVertexDesc, ARRAYSIZE(particleVertexDesc)));

	dirt = new ParticleSystem(device, dirtEffect, dirtMat);
	dirt->setWorldMatrix(XMMatrixScaling(0.7f, 1.5f, 0.7f)*
		XMMatrixTranslation(-1.0f, grass->CalculateYValueWorld(-1.0f, 0.0f) + 0.3, 0.0f));
	dirt->update(context);

	shared_ptr<Material> smokeMat(new Material(device));
	Texture smokeTexture = Texture(device, L"..\\Resources\\Textures\\Smoke.tif");
	smokeMat->setTexture(smokeTexture.getShaderResourceView());

	shared_ptr<Effect> smokeEffect(new Effect(device, "Shaders\\cso\\fire_vs.cso",
		"Shaders\\cso\\fire_ps.cso",
		particleVertexDesc, ARRAYSIZE(particleVertexDesc)));

	smoke = new ParticleSystem(device, smokeEffect, smokeMat);
	smoke->setWorldMatrix(XMMatrixScaling(2.0f, 3.0f, 3.0f)*
		XMMatrixTranslation(-1.0f, grass->CalculateYValueWorld(-1.0f, 0.0f) + 1.3f, 0.0f));
	smoke->update(context);

	// Don't write transparent objects to depth buffer
	dirtEffect->setDepthWriteMask(device, D3D11_DEPTH_WRITE_MASK_ZERO);
	smokeEffect->setDepthWriteMask(device, D3D11_DEPTH_WRITE_MASK_ZERO);

	// Create lens flares
	shared_ptr<Effect> flareEffect(new Effect(device, "Shaders\\cso\\flare_vs.cso",
		"Shaders\\cso\\flare_ps.cso",
		flareVertexDesc, ARRAYSIZE(flareVertexDesc)));

	// Create custom flare blend state
	ID3D11BlendState* flareBlendState = flareEffect->getBlendState();
	D3D11_BLEND_DESC blendDesc;
	flareBlendState->GetDesc(&blendDesc);
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	flareBlendState->Release();
	device->CreateBlendState(&blendDesc, &flareBlendState);
	flareEffect->setBlendState(flareBlendState);

	// Load flare textures
	Texture flare0Texture = Texture(device, L"..\\Resources\\Textures\\flares\\corona.png");
	Texture flare1Texture = Texture(device, L"..\\Resources\\Textures\\flares\\divine.png");
	Texture flare2Texture = Texture(device, L"..\\Resources\\Textures\\flares\\extendring.png");

	shared_ptr<Material> flareMat0(new Material(device, XMFLOAT4(1, 1, 1, (float)0 / numFlares)));
	flareMat0->setTexture(flare0Texture.getShaderResourceView());

	// Create flares
	flares = new Flare(XMFLOAT3(-1250.0, 520.0, 1000.0), device, flareEffect, flareMat0);

	for (int i = 1; i < numFlares; i++)
	{
		shared_ptr<Material> flareInstanceMat(new Material(device,
			XMFLOAT4(randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, (float)i / numFlares)));

		if (randM1P1() > 0.0f)
			flareInstanceMat->setTexture(flare1Texture.getShaderResourceView());
		else
			flareInstanceMat->setTexture(flare2Texture.getShaderResourceView());

		flares->instances.push_back(Instance(XMMatrixIdentity(), flareInstanceMat));
	}

	// Initialize PhysX karts scene
	PXScene->initScenePX();

	// Load models from Scene.xml (created with LevelEditor)
	load("..\\Resources\\Levels\\Scene01_RH_X2.xml", perPixelLightingEffect);

	// Create PhysX terrain from heightfield
	PXScene->CreateHeightField(grass->getHeightArray(), terrainSizeWH, terrainScaleXZ, terrainScaleY);

	// Load karts
	kartWraps.push_back("GreyCamo.png");
	kartWraps.push_back("RedCamo.png");
	kartWraps.push_back("YellowCamo.png");
	kartWraps.push_back("GreenCamo.png");
	kartWraps.push_back("BlueCamo.png");
	kartWraps.push_back("PurpleCamo.png");
	kartWraps.push_back("WhiteStripe.png");
	kartWraps.push_back("BlackStripe.png");
	kartWraps.push_back("RedStripe.png");
	kartWraps.push_back("YellowStripe.png");
	kartWraps.push_back("GreenStripe.png");
	kartWraps.push_back("BlueStripe.png");
	kartWraps.push_back("PurpleStripe.png");

	for (int i = 0; i < kartWraps.size(); i++)
	{
		shared_ptr<Texture> tmpTex(new Texture(device,
			StringToWString(string("..\\Resources\\Models\\Kart\\" + kartWraps[i]))));
		kartTextures.push_back(tmpTex);
	}

	// Create karts with random wraps
	for (int i = 0; i < NUM_AI_VEHICLES - 1; i++)
		aiKarts[i] = new AIKart(FMSystem, device, context, grass, navPoints,
			perPixelLightingEffect, kartTextures[(int)(rand021() * (float)kartTextures.size())]);

	aiKarts[NUM_AI_VEHICLES - 1] = new AIKart(FMSystem, device, context, grass, navPoints,
		beeEffect, kartTextures[(int)(rand021() * (float)kartTextures.size())]);

	int playerKartWrap = (int)(rand021() * (float)kartTextures.size());
	playerKart = new PlayerKart(FMSystem, device, context, grass,
		perPixelLightingEffect, kartTextures[playerKartWrap]);

	menu->setPlayerKartTextures(&kartWraps, &kartTextures, playerKartWrap);

	// Assign karts to array for batch PhysX updates
	kartArray[0] = playerKart;
	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		kartArray[i + 1] = aiKarts[i];

	physx::PxVec3 pP(36.0f, 0, 1.2f);
	pP.y = grass->CalculateYValueWorld(pP.x, pP.z) + pP.y;

	// Initialize PhysX vehicle controller
	PXKartController = new PhysXVehicleController();
	playerKart->setVehicle4W(PXKartController->initVehiclePX(pP.x, pP.y, pP.z, -100.0f, 0));
	playerKart->setStartPosition(pP, 0);

	for (int i = 0; i < NUM_AI_VEHICLES; i++)
	{
		aiKarts[i]->setVehicle4W(PXKartController->initVehiclePX(
			pP.x + floor((i + 1) / 2) * 2, pP.y, pP.z - 2.0f * ((i + 1) % 2), -100.0f, i + 1));
		aiKarts[i]->setStartPosition(pP, i + 1);
	}

	// Setup main camera
	mainCamera = new FirstPersonCamera(device, XMVectorSet(-9.0, 2.0, 17.0, 1.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f),
		XMVectorSet(0.8f, 0.0f, -1.0f, 1.0f));

	mainCamera->setFlying(false);
	float cutOff = cos(XMConvertToRadians(45));
	cout << "cutOff" << cutOff << endl;

	// Setup light constant buffers
	cBufferLightCPU = (CBufferLight*)_aligned_malloc(sizeof(CBufferLight) * MAX_LIGHTS, 16);

	// Fill out light properties
	cBufferLightCPU[0].lightVec = XMFLOAT4(-1250.0, 1000.0, 5.0, 1.0);
	cBufferLightCPU[0].lightAmbient = XMFLOAT4(0.3, 0.3, 0.5, 1.0);
	cBufferLightCPU[0].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[0].lightSpecular = XMFLOAT4(0.9, 0.9, 0.9, 1.0);
	cBufferLightCPU[0].lightAttenuation = XMFLOAT4(1.0, 0.0, 0.0, 10000.0);
	cBufferLightCPU[0].lightCone = XMFLOAT4(0.0, -1.0, 0.0, 0);

	// Additional lights setup...
	cBufferLightCPU[1].lightVec = XMFLOAT4(0, gLightDistance * 0.625, gLightDistance, 1.0);
	cBufferLightCPU[1].lightAmbient = XMFLOAT4(0.0, 0.0, 0.0, gRed);
	cBufferLightCPU[1].lightDiffuse = XMFLOAT4(2.5, 2.5, 2.5, 1.0);
	cBufferLightCPU[1].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[1].lightAttenuation = XMFLOAT4(0.01, 0.0, 0.9, 10.0);
	cBufferLightCPU[1].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

	cBufferLightCPU[2].lightVec = XMFLOAT4(0, 1, 0, 1.0);
	cBufferLightCPU[2].lightAmbient = XMFLOAT4(0.3, 0.3, 0.3, 1.0);
	cBufferLightCPU[2].lightDiffuse = XMFLOAT4(1.0, 1.0,1.0, 1.0);
	cBufferLightCPU[2].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[2].lightAttenuation = XMFLOAT4(1.0, 0.2, 0.1, 10.0);
	cBufferLightCPU[2].lightCone = XMFLOAT4(0.0, -1.0, 0.0, 0);

	cBufferLightCPU[3].lightVec = XMFLOAT4(0, 10, 15, 1.0);
	cBufferLightCPU[3].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
	cBufferLightCPU[3].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[3].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[3].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
	cBufferLightCPU[3].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

	cBufferLightCPU[4].lightVec = XMFLOAT4(0, 10, 30, 1.0);
	cBufferLightCPU[4].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
	cBufferLightCPU[4].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[4].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[4].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
	cBufferLightCPU[4].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

	cBufferLightCPU[5].lightVec = XMFLOAT4(-40, 10, 0, 1.0);
	cBufferLightCPU[5].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
	cBufferLightCPU[5].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[5].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[5].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
	cBufferLightCPU[5].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

	cBufferLightCPU[6].lightVec = XMFLOAT4(-80, 10, 0, 1.0);
	cBufferLightCPU[6].lightAmbient = XMFLOAT4(0.3, 0.3, 0.3, 1.0);
	cBufferLightCPU[6].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[6].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[6].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
	cBufferLightCPU[6].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

	cBufferLightCPU[7].lightVec = XMFLOAT4(40, 10, 0, 1.0);
	cBufferLightCPU[7].lightAmbient = XMFLOAT4(0.3, 0.1, 0.0, 1.0);
	cBufferLightCPU[7].lightDiffuse = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[7].lightSpecular = XMFLOAT4(1.0, 0.3, 0.0, 1.0);
	cBufferLightCPU[7].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
	cBufferLightCPU[7].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);

	cBufferLightCPU[8].lightVec = XMFLOAT4(80, 10, 0, 1.0);
	cBufferLightCPU[8].lightAmbient = XMFLOAT4(0.3, 0.3, 0.0, 1.0);
	cBufferLightCPU[8].lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[8].lightSpecular = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	cBufferLightCPU[8].lightAttenuation = XMFLOAT4(1.0, 0.1, 0.05, 10.0);
	cBufferLightCPU[8].lightCone = XMFLOAT4(0.0, -1.0, 0.0, cutOff);
	
	// Create GPU constant buffer
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));

	cbufferDesc.ByteWidth = sizeof(CBufferLight) * numLights;
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferInitData.pSysMem = cBufferLightCPU;

	HRESULT hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferLightGPU);

	// Map buffer to GPU
	mapCbuffer(context, cBufferLightCPU, cBufferLightGPU, sizeof(CBufferLight) * numLights);
	context->VSSetConstantBuffers(2, 1, &cBufferLightGPU);
	context->PSSetConstantBuffers(2, 1, &cBufferLightGPU);

	// Scene constant buffer setup
	cBufferSceneCPU = (CBufferScene*)_aligned_malloc(sizeof(CBufferScene), 16);

	// Fill out scene properties
	cBufferSceneCPU->windDir = XMFLOAT4(1, 0, 0, 1);
	cBufferSceneCPU->Time = 0.0;
	cBufferSceneCPU->grassHeight = 0.2;
	cBufferSceneCPU->USE_SHADOW_MAP = true;
	cBufferSceneCPU->QUALITY = menu->getQuality();
	cBufferSceneCPU->fog = 0.15f;
	cBufferSceneCPU->numLights = numLights;

	cbufferInitData.pSysMem = cBufferSceneCPU;
	cbufferDesc.ByteWidth = sizeof(CBufferScene);

	hr = device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSceneGPU);

	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	context->VSSetConstantBuffers(3, 1, &cBufferSceneGPU);
	context->PSSetConstantBuffers(3, 1, &cBufferSceneGPU);

	// Enable main menu
	MainMenuState = MainMenu;

	return S_OK;
}

// Lens flare rendering implementation
void Scene::DrawFlare(ID3D11DeviceContext* context)
{
	if (flares) {
		// Set NULL depth buffer to use Depth Buffer as shader resource
		ID3D11RenderTargetView* tempRT = system->getBackBufferRTV();
		context->OMSetRenderTargets(1, &tempRT, NULL);

		ID3D11ShaderResourceView* depthSRV = system->getDepthStencilSRV();
		context->VSSetShaderResources(5, 1, &depthSRV);

		flares->renderInstances(context);

		// Release depth shader resource
		ID3D11ShaderResourceView* nullSRV = NULL;
		context->VSSetShaderResources(5, 1, &nullSRV);

		// Restore default depth buffer view
		tempRT = system->getBackBufferRTV();
		context->OMSetRenderTargets(1, &tempRT, system->getDepthStencil());
	}
}

// Quality settings adjustment based on menu selection
void Scene::UpdateRenderQuality()
{
	stepSize = 1.0f / 60.0f;

	if (menu->getQuality() == LOWEST)
	{
		stepSize = 1.0f / 25.0f;
		numGrassPasses = 1;
		numPXUpdates = 1;
		numKartPXUpdates = 2;
	}
	else if (menu->getQuality() == LOW)
	{
		stepSize = 1.0f / 30.0f;
		numGrassPasses = 3;
		numPXUpdates = 2;
	}
	else if (menu->getQuality() == MEDIUM)
	{
		stepSize = 1.0f / 30.0f;
		numGrassPasses = 5;
		numPXUpdates = 2;
	}
	else if (menu->getQuality() == HIGH)
	{
		stepSize = 1.0f / 50.0f;
		numGrassPasses = 10;
		numPXUpdates = 2;
	}
	else if (menu->getQuality() == HIGHEST)
	{
		stepSize = 1.0f / 60.0f;
		numGrassPasses = 20;
		numPXUpdates = 2;
	}

	// Update shadow map size based on quality
	if (cBufferSceneCPU->QUALITY != menu->getQuality())
	{
		cBufferSceneCPU->QUALITY = menu->getQuality();
		mapCbuffer(system->getDeviceContext(), cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

		int shadowMapWidth = 2048;
		if (menu->getQuality() == MEDIUM)
			shadowMapWidth = 4096;
		else if (menu->getQuality() > MEDIUM)
			shadowMapWidth = 8192;

		shadowMap->setMapSize(system->getDevice(), shadowMapWidth);
	}
}

// Main scene update with physics and animation
HRESULT Scene::updateScene(ID3D11DeviceContext* context, FirstPersonCamera* camera) {
	// Update clock
	mainClock->tick();
	double dT = mainClock->gameTimeDelta();
	double gT = mainClock->gameTimeElapsed();
	acc += dT;

	// Fixed timestep implementation
	if (acc < stepSize)
	{
		return S_OK;
	}

	renderFrame = true;
	acc = fmod(acc, stepSize);

	// Update scene time for animations
	cBufferSceneCPU->Time = gT;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

	// Start AI karts with delays
	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		if (gT > i * 8 + 5) aiKarts[i]->setStarted(true);

	// Update physics with multiple steps for stability
	for (int i = 0; i < numPXUpdates; i++) {
		for (int i = 0; i < numKartPXUpdates; i++)
			PXKartController->stepVehicles(stepSize * 1.6f / (numPXUpdates * numKartPXUpdates), kartArray, NUM_VEHICLES);

		for (int i = 0; i < numScenePXUpdates; i++)
			PXScene->step(stepSize * 1.6f / (numPXUpdates * numScenePXUpdates));
	}

	// Update kart graphics
	playerKart->update(context, stepSize);
	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		aiKarts[i]->update(context, stepSize);

	// Update PhysX boxes graphics
	for (int i = 0; i < S; i++)
	{
		physx::PxTransform boxT = body[i]->getGlobalPose();
		XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(boxT.q.x, boxT.q.y, boxT.q.z, boxT.q.w));
		boxMatArray[i] = XMMatrixScaling(boxHalfSize, boxHalfSize, boxHalfSize) *
			XMMatrixRotationQuaternion(quart) *
			XMMatrixTranslation(boxT.p.x * 1.0, boxT.p.y + 0.2, boxT.p.z * 1.0);
	}

	// Update lap timer
	for (int i = 0; i < NUM_VEHICLES; i++)
		kartArray[i]->updateLapTimes(stepSize, startDist, finishDist, startFinishPos);

	// Update player camera
	if (!mainCamera->getFlying())
		updatePlayerCamera(context, camera);

	// Update dragon animation and movement
	float r = 0;
	if (dragon->getCurrentAnim() == 2) r = -0.4 * stepSize;
	else if (dragon->getCurrentAnim() == 3) r = -0.2 * stepSize;
	else r = -0.2 * stepSize;

	dragon->setWorldMatrix(dragon->getWorldMatrix() *
		XMMatrixTranslation(20, 0, 30) *
		XMMatrixRotationY(r) *
		XMMatrixTranslation(-20, 0, -30));

	XMVECTOR dragonPos = XMVectorZero();
	dragonPos = XMVector3TransformCoord(dragonPos, dragon->getWorldMatrix());
	float dragonHeight = grass->CalculateYValueWorld(DirectX::XMVectorGetX(dragonPos), DirectX::XMVectorGetZ(dragonPos));

	dragon->setWorldMatrix(dragon->getWorldMatrix() *
		XMMatrixTranslation(0, dragonHeight - DirectX::XMVectorGetY(dragonPos), 0));

	dragon->update(context);
	dragon->updateBones(gT);

	// Update Nathan animation
	nathan->setWorldMatrix(nathan->getWorldMatrix() *
		XMMatrixTranslation(0, 0, 0) *
		XMMatrixRotationY(r) *
		XMMatrixTranslation(0, 0, 0));

	nathan->update(context);
	nathan->updateBonesSubFrames(19, 52, gT);

	// Update Sophia animation
	sophia->update(context);
	sophia->updateBones(gT);

	// Time of day lighting changes
	float tod = sin(mainClock->gameTimeElapsed() / 20.0f) / 2 + 0.5f;
	float todBlue = tod * 0.5 + 0.5;

	cBufferLightCPU[0].lightAmbient = XMFLOAT4(0.3 * tod, 0.3 * tod, 0.3 * todBlue, 1.0);
	cBufferLightCPU[0].lightDiffuse = XMFLOAT4(0.8 * tod, 0.8 * tod, 1.0 * todBlue, 1.0);
	cBufferLightCPU[0].lightSpecular = XMFLOAT4(0.8 * tod, 0.8 * tod, 1.0 * todBlue, 1.0);

	// Animate light position
	XMMATRIX rotM = XMMatrixRotationZ(gT) * XMMatrixTranslation(0, 5, 0);
	cBufferLightCPU[1].lightVec.x = gLightDistance * 0.625;
	cBufferLightCPU[1].lightVec.z = gLightDistance;
	cBufferLightCPU[1].lightVec.y = 0;

	XMVECTOR lightVec = DirectX::XMLoadFloat4(&(cBufferLightCPU[1].lightVec));
	lightVec = DirectX::XMVector3TransformCoord(lightVec, rotM);
	DirectX::XMStoreFloat4(&(cBufferLightCPU[1].lightVec), lightVec);

	orb3->setWorldMatrix(XMMatrixScaling(0.5, 0.5, 0.5) *
		XMMatrixTranslation(cBufferLightCPU[1].lightVec.x,
			cBufferLightCPU[1].lightVec.y,
			cBufferLightCPU[1].lightVec.z));

	orb3->update(context);

	// Update light positions based on kart positions
	physx::PxTransform trans = aiKarts[NUM_AI_VEHICLES - 1]->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
	trans.p.y += 0.05;

	cBufferLightCPU[2].lightVec.x = trans.p.x;
	cBufferLightCPU[2].lightVec.y = trans.p.y + 1;
	cBufferLightCPU[2].lightVec.z = trans.p.z;

	// Update GPU light buffer
	mapCbuffer(context, cBufferLightCPU, cBufferLightGPU, sizeof(CBufferLight) * numLights);

	return S_OK;
}

// Camera update following player kart
void Scene::updatePlayerCamera(ID3D11DeviceContext* context, FirstPersonCamera* camera)
{
	physx::PxTransform trans = playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
	trans.p.y += 0.05;

	if (playerKart->camMode == 3) // Birds eye view
	{
		// Top down view
		camera->setPos(XMVectorSet(trans.p.x, 15, trans.p.z, 1.0));
		camera->setLookAt(XMVectorSet(trans.p.x, 0, trans.p.z, 1));
		camera->setUp(XMVectorSet(0, 0, 1, 1));
	}
	else {
		XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(trans.q.x, trans.q.y, trans.q.z, trans.q.w));
		XMVECTOR campos = XMVectorSet(trans.p.x * 1.0, trans.p.y * 1.0 + 2.0f, trans.p.z * 1.0, 1.0);
		XMVECTOR dir = XMVectorSet(0, 0, 1.0, 1);
		static XMVECTOR oldDir = dir;
		dir = DirectX::XMVector4Transform(dir, XMMatrixRotationQuaternion(quart));
		static XMVECTOR oldcampos = camera->getPos();
		XMVECTOR target;
		float timeScale = stepSize * 60.0f;

		if (playerKart->camMode == 0) // First Person Camera
		{
			// Lerp between old and new camera positions for smooth movement
			float currentWeightDIR = 0.5 * timeScale;
			float currentWeightPOS = 0.7 * timeScale;
			target = DirectX::XMVectorSubtract(campos, DirectX::XMVectorScale(dir, 0.6f));
			dir = DirectX::XMVectorAdd(DirectX::XMVectorScale(dir, currentWeightDIR),
				DirectX::XMVectorScale(oldDir, 1.0f - currentWeightDIR));

			oldDir = dir;
			camera->setPos(DirectX::XMVectorAdd(DirectX::XMVectorScale(target, currentWeightPOS),
				DirectX::XMVectorScale(oldcampos, 1.0f - currentWeightPOS)));

			campos = XMVectorSet(trans.p.x, trans.p.y + 1.0f, trans.p.z, 1.0);
			camera->setHeight(DirectX::XMVectorGetY(campos));
			campos = DirectX::XMVectorSetY(campos, DirectX::XMVectorGetY(campos) - 1);
			camera->setLookAt(DirectX::XMVectorAdd(campos, DirectX::XMVectorScale(dir, 6.0)));
			camera->setUp(XMVectorSet(0, 1, 0, 1));
		}
		else if (playerKart->camMode == 1) // Third Person Camera close
		{
			float currentWeightDIR = 0.1 * timeScale;
			float currentWeightPOS = 0.1 * timeScale;
			target = DirectX::XMVectorSubtract(campos, DirectX::XMVectorScale(dir, 2.0f));
			dir = DirectX::XMVectorAdd(DirectX::XMVectorScale(dir, currentWeightDIR),
				DirectX::XMVectorScale(oldDir, 1.0f - currentWeightDIR));

			oldDir = dir;
			camera->setPos(DirectX::XMVectorAdd(DirectX::XMVectorScale(target, currentWeightPOS),
				DirectX::XMVectorScale(oldcampos, 1.0f - currentWeightPOS)));

			campos = XMVectorSet(trans.p.x, trans.p.y + 1.0f, trans.p.z, 1.0);
			camera->setHeight(DirectX::XMVectorGetY(campos));
			camera->setLookAt(campos);
			camera->setUp(XMVectorSet(0, 1, 0, 1));
		}
		else if (playerKart->camMode == 2) // Third Person Camera far
		{
			float currentWeightDIR = 0.05 * timeScale;
			float currentWeightPOS = 0.05 * timeScale;
			target = DirectX::XMVectorSubtract(campos, DirectX::XMVectorScale(dir, 2.0f));
			dir = DirectX::XMVectorAdd(DirectX::XMVectorScale(dir, currentWeightDIR),
				DirectX::XMVectorScale(oldDir, 1.0f - currentWeightDIR));

			oldDir = dir;
			camera->setPos(DirectX::XMVectorAdd(DirectX::XMVectorScale(target, currentWeightPOS),
				DirectX::XMVectorScale(oldcampos, 1.0f - currentWeightPOS)));

			campos = XMVectorSet(trans.p.x, trans.p.y + 1.0f, trans.p.z, 1.0);
			camera->setHeight(DirectX::XMVectorGetY(campos));
			camera->setLookAt(campos);
			camera->setUp(XMVectorSet(0, 1, 0, 1));
		}
		oldcampos = camera->getPos();
	}
	camera->update(context);
}
// Main rendering routine
HRESULT Scene::renderScene() {
	ID3D11DeviceContext* context = system->getDeviceContext();

	// Validate window and D3D context
	if (isMinimised() || !context)
		return E_FAIL;

	if (renderFrame == false)
		return S_OK;

	// Shadow mapping pass
	if (true)
	{
		// Disable shadow map for shadow pass
		cBufferSceneCPU->USE_SHADOW_MAP = false;
		mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

		shadowMap->update(context);

		// Disable alpha to coverage for shadow map (not multisampled)
		tree->getEffect()->setAlphaToCoverage(system->getDevice(), FALSE);
		models[1]->getEffect()->setAlphaToCoverage(system->getDevice(), FALSE);

		// Render objects to shadow map
		shadowMap->render(system, std::bind(&Scene::renderShadowObjects, this, std::placeholders::_1));

		// Re-enable shadow map
		cBufferSceneCPU->USE_SHADOW_MAP = true;
		mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

		// Re-enable alpha to coverage
		tree->getEffect()->setAlphaToCoverage(system->getDevice(), TRUE);
		models[1]->getEffect()->setAlphaToCoverage(system->getDevice(), TRUE);

		// Update dynamic cube map
		dynamicCubeMap->updateCubeCameras(context,
			XMVectorSet(XMVectorGetX(mainCamera->getPos()),
				-XMVectorGetY(mainCamera->getPos()) + 1.8,
				XMVectorGetZ(mainCamera->getPos()), 1));

		dynamicCubeMap->render(system, std::bind(&Scene::renderDynamicObjects, this, std::placeholders::_1));

		// Restore cube environment texture
		ID3D11ShaderResourceView* cubeDayTextureSRV = cubeDayTexture->getShaderResourceView();
		context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);

		// Restore main camera
		mainCamera->update(context);
	}
	else
	{
		cBufferSceneCPU->USE_SHADOW_MAP = true;
		mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	}

	// Clear the back buffer
	static const FLOAT clearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	context->ClearRenderTargetView(system->getBackBufferRTV(), clearColor);
	context->ClearDepthStencilView(system->getDepthStencil(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render scene objects
	renderSceneObjects(context);

	// Apply bloom effect to glow objects
	bloom->blurModels(system, std::bind(&Scene::renderGlowObjects, this, std::placeholders::_1));

	// Draw lens flare
	renderGlowObjects(context);

	// Draw flare based on time of day
	if (sin(mainClock->gameTimeElapsed() / 20.0f) > 0.5)
		DrawFlare(context);

	// Draw HUD
	menu->renderGUIMenu(&MainMenuState, kartArray, NUM_VEHICLES);

	// Present to screen
	HRESULT hr = system->presentBackBuffer();

	return S_OK;
}

// Render glow objects for bloom effect
void Scene::renderGlowObjects(ID3D11DeviceContext* context)
{
	orb3->renderInstances(context);
	aiKarts[NUM_AI_VEHICLES - 1]->render(context);
}

// Render dynamic objects for cube map
void Scene::renderDynamicObjects(ID3D11DeviceContext* context)
{
	ID3D11ShaderResourceView* cubeDayTextureSRV = cubeDayTexture->getShaderResourceView();
	context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);

	skyBox->render(context);
	models[7]->renderInstances(context);
	models[9]->renderInstances(context);

	playerKart->render(context, playerKart->camMode);

	// Render ground base
	cBufferSceneCPU->grassHeight = 0.0f;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	grass->getEffect()->setDepthWriteMask(system->getDevice(), D3D11_DEPTH_WRITE_MASK_ALL);
	grass->render(context);
}

// Render objects to shadow map
void Scene::renderShadowObjects(ID3D11DeviceContext* context)
{
	if (cBufferSceneCPU->USE_SHADOW_MAP == false)
	{
		physXBox->getEffect()->setCullMode(system->getDevice(), D3D11_CULL_BACK);
		playerKart->getFarings()->getEffect()->setCullMode(system->getDevice(), D3D11_CULL_BACK);
	}

	// Render PhysX boxes
	if (physXBox)
		for (int i = 0; i < S; i++)
		{
			physXBox->setWorldMatrix(boxMatArray[i]);
			physXBox->update(context);
			physXBox->render(context);
		}

	// Render karts
	playerKart->render(context, playerKart->camMode);
	for (int i = 0; i < NUM_AI_VEHICLES - 1; i++)
		aiKarts[i]->render(context);

	// Render tree instances
	if (tree)
		tree->renderInstances(context);

	// Render loaded models
	for (int i = 1; i < models.size(); i++)
		models[i]->renderInstances(context);

	// Render animated characters
	dragon->render(context);
	nathan->renderInstances(context);
	sophia->renderInstances(context);

	if (cBufferSceneCPU->USE_SHADOW_MAP == false)
	{
		physXBox->getEffect()->setCullMode(system->getDevice(), D3D11_CULL_BACK);
		playerKart->getFarings()->getEffect()->setCullMode(system->getDevice(), D3D11_CULL_BACK);
	}
}

// Render main scene objects
void Scene::renderSceneObjects(ID3D11DeviceContext* context)
{
	// Render skybox
	if (skyBox)
		skyBox->render(context);

	renderShadowObjects(context);

	// Render ground base
	cBufferSceneCPU->grassHeight = 0.0f;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	grass->getEffect()->setDepthWriteMask(system->getDevice(), D3D11_DEPTH_WRITE_MASK_ALL);
	grass->render(context);

	// Render racing line
	if (models.size() > 0)
		models[0]->renderInstances(context);

	// Render remaining grass shells with alpha blending
	grass->getEffect()->setAlphaBlendEnable(system->getDevice(), true);
	grass->getEffect()->setDepthWriteMask(system->getDevice(), D3D11_DEPTH_WRITE_MASK_ZERO);

	// Render grass layers from base to tip
	for (int i = 1; i < numGrassPasses; i++)
	{
		cBufferSceneCPU->grassHeight = (grassLength / numGrassPasses) * i;
		mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
		grass->render(context);
	}

	//Render Face
	if (gUseTSD > 0)
		skinSim->blurModel(face, system->getDepthStencilSRV());
	else
		face->render(context);
	
	// Render water with dynamic cube map
	ID3D11ShaderResourceView* dunamicCubeTextureSRV = dynamicCubeMap->getSRV();
	context->PSSetShaderResources(6, 1, &dunamicCubeTextureSRV);

	if (water)
		water->render(context);

	// Render particle effects from kart tires
	if (playerKart->getWheelSpin())
	{
		if (playerKart->getOnGrass())
		{
			dirt->setWorldMatrix(playerKart->getLSmokeMat());
			dirt->update(context);
			dirt->render(context);

			dirt->setWorldMatrix(playerKart->getRSmokeMat());
			dirt->update(context);
			dirt->render(context);
		}
		else
		{
			smoke->setWorldMatrix(playerKart->getLSmokeMat());
			smoke->update(context);
			smoke->render(context);

			smoke->setWorldMatrix(playerKart->getRSmokeMat());
			smoke->update(context);
			smoke->render(context);
		}
	}
}

// Input handling methods
void Scene::handleMouseLDrag(const POINT& disp) {
	if (MainMenuState != MenuDone)
	{
		menuKartRot += -disp.x * 0.01f;
	}
	else if (mainCamera->getFlying())
	{
		mainCamera->elevate((float)-disp.y * 0.01f);
		mainCamera->turn((float)disp.x * 0.01f);
	}
}

void Scene::handleMouseWheel(const short zDelta) {
	if (MainMenuState >= MenuDone && mainCamera->getFlying())
		mainCamera->move(-zDelta * 0.01);
}

void Scene::handleKeyDown(const WPARAM keyCode, const LPARAM extKeyFlags) {
	if (keyCode == 'f' || keyCode == 'F')
		mainCamera->toggleFlying();

	if (keyCode == 'w' || keyCode == 'W')
		if (mainCamera->getFlying())
			mainCamera->move(0.5);

	if (keyCode == 's' || keyCode == 'S')
		if (mainCamera->getFlying())
			mainCamera->move(-0.5);

	if (keyCode == 'i' || keyCode == 'I')
	{
		if (gLightDistance > 1.28f)
			gLightDistance = gLightDistance / 2.0;
		cout << "LightPos: " << cBufferLightCPU->lightVec.z << endl;
	}

	if (keyCode == 'l' || keyCode == 'L')
	{
		gRed += 0.4f;
		cout << "Life: " << gRed << endl;
		cBufferLightCPU[1].lightAmbient.w = gRed;
	}

	if (keyCode == 'k' || keyCode == 'K')
	{
		if (gRed > 1.0f)
			gRed -= 0.4f;
		cBufferLightCPU[1].lightAmbient.w = gRed;
		cout << "Life: " << gRed << endl;
	}

	if (keyCode == 'o' || keyCode == 'O')
	{
		if (gLightDistance < 8.0f)
			gLightDistance = gLightDistance * 2.0;
		cout << "LightPos: " << cBufferLightCPU->lightVec.z << endl;
	}

	if ((keyCode == 't' || keyCode == 'T'))
		gUseTSD *= -1;

	if (keyCode == VK_ESCAPE)
		MainMenuState = MainMenu;

	if (keyCode == VK_PRIOR)
		playerKart->camMode = max(playerKart->camMode--, 0);

	if (keyCode == VK_NEXT)
		playerKart->camMode = min(playerKart->camMode++, 3);

	if (keyCode == VK_SPACE)
	{
		playerKart->restart(playerKart->getPolePosition());
		playerKart->setLapTime(0.0f);
	}

	if (keyCode == VK_HOME)
	{
		if (playerKart->getKartCC() < 2)
			playerKart->setKartCC(setCC(playerKart, playerKart->getKartCC() + 1));
	}

	if (keyCode == VK_END)
	{
		if (playerKart->getKartCC() > 0)
			playerKart->setKartCC(setCC(playerKart, playerKart->getKartCC() - 1));
	}
}

void Scene::handleKeyUp(const WPARAM keyCode, const LPARAM extKeyFlags) {
	// Key up handling
}

// Clock handling methods
void Scene::startClock() {
	mainClock->start();
}

void Scene::stopClock() {
	mainClock->stop();
}

void Scene::reportTimingData() {
	cout << "Actual time elapsed = " << mainClock->actualTimeElapsed() << endl;
	cout << "Game time elapsed = " << mainClock->gameTimeElapsed() << endl << endl;
	mainClock->reportTimingData();
}

// Constructor with window creation and DirectX initialization
Scene::Scene(const LONG _width, const LONG _height, const wchar_t* wndClassName,
	const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {
	try
	{
		// 1. Register window class
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_DBLCLKS | CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_CROSS);
		wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = wndClassName;
		wcex.hIconSm = NULL;

		if (!RegisterClassEx(&wcex))
			throw exception("Cannot register window class for Scene HWND");

		// 2. Store instance handle
		hInst = hInstance;

		// 3. Setup window rect
		RECT windowRect;
		windowRect.left = 0;
		windowRect.right = _width;
		windowRect.top = 0;
		windowRect.bottom = _height;

		DWORD dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
		DWORD dwStyle = WS_OVERLAPPEDWINDOW;

		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		// 4. Create and validate main window
		wndHandle = CreateWindowEx(dwExStyle, wndClassName, wndTitle,
			dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			50, 50, windowRect.right - windowRect.left,
			windowRect.bottom - windowRect.top,
			NULL, NULL, hInst, this);

		if (!wndHandle)
			throw exception("Cannot create main window handle");

		ShowWindow(wndHandle, nCmdShow);
		UpdateWindow(wndHandle);
		SetFocus(wndHandle);

		// 5. Create DirectX system
		system = System::CreateDirectXSystem(wndHandle);
		if (!system)
			throw exception("Cannot create Direct3D device and context model");

		// 6. Create main clock
		mainClock = CGDClock::CreateClock(string("mainClock"), 3.0f);
		if (!mainClock)
			throw exception("Cannot create main clock / timer");

		// 7. Setup application-specific objects
		HRESULT hr = initialiseSceneResources();
		if (!SUCCEEDED(hr))
			throw exception("Cannot initalise scene resources");
	}
	catch (exception& e)
	{
		cout << e.what() << endl;
		throw;
	}
}

// Main game loop implementation
HRESULT Scene::updateAndRenderScene() {
	HRESULT hr = S_OK;
	ID3D11DeviceContext* context = system->getDeviceContext();

	if (MainMenuState < MenuDone)
	{
		// Rotate kart for menu display
		menuKartRot += 0.01;
		PxQuat Q(menuKartRot, PxVec3(0, 1, 0));
		physx::PxTransform startTransform(physx::PxVec3(0, -0.9f, 0), Q);
		static physx::PxTransform gameTransform = playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();

		playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(startTransform);
		playerKart->update(context, stepSize);

		// Show main menu
		menu->renderMainMenu(playerKart, &MainMenuState, stepSize);
		UpdateRenderQuality();

		playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(gameTransform);
	}
	else if (MainMenuState >= MenuDone)
	{
		hr = updateScene(context, mainCamera);
		if (SUCCEEDED(hr))
			hr = renderScene();
	}

	return hr;
}

// Window state check
BOOL Scene::isMinimised() {
	WINDOWPLACEMENT wp;
	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);

	return (GetWindowPlacement(wndHandle, &wp) != 0 && wp.showCmd == SW_SHOWMINIMIZED);
}

// Factory method pattern
Scene* Scene::CreateScene(const LONG _width, const LONG _height, const wchar_t* wndClassName,
	const wchar_t* wndTitle, int nCmdShow, HINSTANCE hInstance, WNDPROC WndProc) {
	static bool _scene_created = false;
	Scene* scene = nullptr;

	if (!_scene_created) {
		scene = new Scene(_width, _height, wndClassName, wndTitle, nCmdShow, hInstance, WndProc);
		if (scene)
			_scene_created = true;
	}

	return scene;
}

// Destructor with comprehensive cleanup
Scene::~Scene() {
	cout << "Destroying scene" << endl;

	// Clean up menu
	delete(menu);

	// Clean up shadow map
	delete(shadowMap);

	// Clean up constant buffers
	if (cBufferSceneCPU)
		_aligned_free(cBufferSceneCPU);

	if (cBufferLightCPU)
		_aligned_free(cBufferLightCPU);

	if (cBufferSceneGPU)
		cBufferSceneGPU->Release();

	if (cBufferLightGPU)
		cBufferLightGPU->Release();

	// Clean up textures
	if (cubeDayTexture)
		delete(cubeDayTexture);

	// Clean up scene objects
	if (navPoints)
		delete(navPoints);

	if (skyBox)
		delete(skyBox);

	if (water)
		delete(water);

	if (orb3)
		delete(orb3);

	if (grass)
		delete(grass);

	if (physXBox)
		delete(physXBox);

	if (dragon)
		delete(dragon);

	if (nathan)
		delete(nathan);

	if (sophia)
		delete(sophia);

	if (tree)
		delete(tree);

	if (dirt)
		delete(dirt);

	if (smoke)
		delete(smoke);

	if (flares)
		delete(flares);

	// Clean up karts
	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		if (aiKarts[i])
			delete(aiKarts[i]);

	if (playerKart)
		delete(playerKart);

	if (PXKartController)
		delete(PXKartController);

	delete[](boxMatArray);

	// Clean up PhysX
	if (PXScene != nullptr) {
		delete PXScene;
		PXScene = nullptr;
	}

	// Clean up FMOD
	if (FMSystem) {
		FMSystem->release();
		FMSystem = nullptr;
	}

	// Clean up core systems
	if (mainClock)
		delete(mainClock);

	if (mainCamera)
		delete(mainCamera);

	if (system)
		delete(system);

	if (wndHandle)
		DestroyWindow(wndHandle);
}

// Window destruction
void Scene::destoryWindow() {
	if (wndHandle != NULL) {
		HWND hWnd = wndHandle;
		wndHandle = NULL;
		DestroyWindow(hWnd);
	}
}

// Resource resizing for window changes
HRESULT Scene::resizeResources() {
	if (system) {
		HRESULT hr = system->resizeSwapChainBuffers(wndHandle);
		rebuildViewport();

		RECT clientRect;
		GetClientRect(wndHandle, &clientRect);

		if (!isMinimised())
			renderScene();
	}

	return S_OK;
}

// DirectInput setup for wheel controller (legacy input)
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib,"Dinput8.lib")
#pragma comment(lib,"Dxguid.lib")

LPDIRECTINPUT8 di;
const GUID LOGITECH_WHEEL_GUID = { 0x046d, 0xc29b };
const GUID LOGITECH_PEDALS_GUID = { 0x046d, 0xc283 };

LPDIRECTINPUTDEVICE8 wheel;
LPDIRECTINPUTDEVICE8 pedals;

BOOL CALLBACK DeviceEnumCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	// Device enumeration for input devices
	WCHAR destString[9];
	const char sourceString[9] = "00CFEF40";
	const int sourceLength = strlen(sourceString) + 1;
	const int destLength = sourceLength;

	MultiByteToWideChar(CP_ACP, 0, sourceString, sourceLength, destString, destLength);

	return DIENUM_CONTINUE;
}

bool IsXboxOneController(LPCDIDEVICEINSTANCE lpddi)
{
	// Check for Xbox One controller
	cout << lpddi->tszProductName << endl;
	return false;
}

BOOL CALLBACK EnumDevicesCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef)
{
	// Enumerate devices and check for Xbox One controller
	if (IsXboxOneController(lpddi))
	{
		if (FAILED(di->CreateDevice(lpddi->guidInstance, &wheel, NULL)))
		{
			return DIENUM_STOP;
		}
	}
	return DIENUM_CONTINUE;
}

void Scene::SetUpWheel()
{
	HINSTANCE hInstance = hInst;

	// Initialize DirectInput
	DirectInput8Create(hInstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&di, nullptr);

	// Find wheel device
	di->EnumDevices(DI8DEVCLASS_GAMECTRL, DeviceEnumCallback, nullptr, DIEDFL_ATTACHEDONLY);

	// Create device and set up data format
	if (wheel)
	{
		wheel->SetDataFormat(&c_dfDIJoystick2);
		wheel->Acquire();
	}
}