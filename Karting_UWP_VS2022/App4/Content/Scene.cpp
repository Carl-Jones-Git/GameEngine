#include "pch.h"
#include "Utils.h"
#include "Effect.h"
#include "Texture.h"
#include "VertexStructures.h"
#include "CBufferStructures.h"
#include "Scene.h"
#include <string.h>
#include <Source\Includes.h>
#include <cstring>
#include <iostream>

#include "..\Common\DirectXHelper.h"
wstring ResourcePath=L"Assets\\";
string ShaderPath = "";
extern PxPhysics* mPhysics;
extern PxScene* mScene;
extern PxCooking* gCooking;



//#include "Box.h"
#include <Material.h>
using namespace App4;

//using namespace DirectX;
using namespace Windows::Foundation;

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib,"Dinput8.lib")
#pragma comment(lib,"Dxguid.lib")

//LPDIRECTINPUT8 di;
// Device GUIDs
const GUID LOGITECH_WHEEL_GUID = { 0x046d, 0xc29b };
const GUID LOGITECH_PEDALS_GUID = { 0x046d, 0xc283 };

//LPDIRECTINPUTDEVICE8 wheel;
//LPDIRECTINPUTDEVICE8 pedals;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Scene::Scene(const std::shared_ptr<DX::DeviceResources>& deviceResources, MoveLookController ^ controller) :
	m_loadingComplete(false),
	m_degreesPerSecond(45),
	m_deviceResources(deviceResources)
{
	m_controller = controller;
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Initializes view parameters when the window size changes.
void Scene::CreateWindowSizeDependentResources()
{
	if (m_loadingComplete)
	{
	ID3D11Device *device = m_deviceResources->GetD3DDevice();
	Size outputSize = m_deviceResources->GetOutputSize();
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 70.0f * XM_PI / 180.0f;

	// This is a simple example of change that can be made when the app is in
	// portrait or snapped view.
	if (aspectRatio < 1.0f)
		fovAngleY *= 2.0f;

	// Note that the OrientationTransform3D matrix is post-multiplied,
	// in order to correctly orient the scene to match the display orientation.
	// This post-multiplication step is required for any draw calls that are
	// made to the swap chain render target. For draw calls to other targets,
	// this transform should not be applied.


	//// Setup a camera
	//// The FirstPersonCamera is derived from the base Camera class. The constructor for the Camera class requires a valid pointer to the main DirectX device
	//// and and 3 vectors to define the initial position, up vector and target for the camera.
	//// The camera class  manages a Cbuffer containing view/projection matrix properties. It has methods to update the cbuffers if the camera moves changes  
	//// The camera constructor and update methods also attaches the camera CBuffer to the pipeline at slot b1 for vertex and pixel shaders

		camera = new FirstPersonCamera(device, XMVectorSet(-9.0, 2.0, 17.0, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorSet(0.8f, 0.0f, -1.0f, 1.0f));
		camera->setFlying(false);
	}
	//// This sample makes use of a right-handed coordinate system using row-major matrices.
	//XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 0.01f, 1000.0f);//Could use near=0.5,far=1000.
	//camera->setProjMatrix(perspectiveMatrix);
	////camera = new FirstPersonCamera(device, XMVectorSet(-9.0, 2.0, 17.0, 1.0f), XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f), XMVectorSet(0.8f, 0.0f, -1.0f, 1.0f), perspectiveMatrix);

	/*m_controller->Pitch(camera->getPitch());
	m_controller->Yaw(camera->getYaw());*/
	//if(!m_controller->Active())
	//	m_controller->WaitForPress(XMFLOAT2(0, 0), XMFLOAT2(outputSize.Width,outputSize.Height));

}

void Scene::updateSceneOrMenu() {


	ID3D11DeviceContext* context = m_deviceResources->GetD3DDeviceContext();

	if (MainMenuState < MenuDone)
	{
		//Rotate the Kart for the mainMenu
		menuKartRot += 0.01;
		PxQuat Q(menuKartRot, PxVec3(0, 1, 0));
		physx::PxTransform startTransform(physx::PxVec3(0, -0.9f, 0), Q);
		static physx::PxTransform gameTransform = playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
		playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(startTransform);
		playerKart->update(context, stepSize);
		//Show the mainMenu
		menu->renderMainMenu(playerKart, &MainMenuState, stepSize);
		//UpdateRenderQuality();
		playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(gameTransform);
	}

	//else if (MainMenuState >= MenuDone)
	//{

	//	hr = updateScene(context, mainCamera);

	//	if (SUCCEEDED(hr))
	//		hr = renderScene();
	//}
	//return hr;
}
// Called once per frame, updates camera and calculates the model and view matrices.
void Scene::Update(DX::StepTimer const& timer)
{
	if (camera==NULL)
		return;
	auto context = m_deviceResources->GetD3DDeviceContext();


	if (MainMenuState < MenuDone)
	{
		//Rotate the Kart for the mainMenu
		menuKartRot += 0.01;
		PxQuat Q(menuKartRot, PxVec3(0, 1, 0));
		physx::PxTransform startTransform(physx::PxVec3(0, -0.9f, 0), Q);
		static physx::PxTransform gameTransform = playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
		playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(startTransform);
		playerKart->update(context, stepSize);
		//UpdateRenderQuality();
		playerKart->getVehicle4W()->getRigidDynamicActor()->setGlobalPose(gameTransform);

		return;
	}

	// update controller
	if (m_controller->IsPressComplete())
		m_controller->Active(true);
	if (m_controller->IsPauseRequested())
		m_controller->WaitForPress(XMFLOAT2(0, 0), XMFLOAT2(m_deviceResources->GetOutputSize().Width, m_deviceResources->GetOutputSize().Height));
	m_controller->Update();

	double dT = 0.0;
	if (!paused)
		dT = timer.GetElapsedSeconds();
	else
		dT = timer.GetElapsedSeconds() / 4.0;
	double gT = timer.GetTotalSeconds();
	cBufferSceneCPU->Time = gT;
	//cBufferSceneCPU->deltaTime = dT;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

	textBuffer = L"FPS = " + std::to_wstring(timer.GetFramesPerSecond());










	// Update the scene time as it is needed to animate the foliage etc
	cBufferSceneCPU->Time = gT;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

	//start the AI karts 1 at a time
	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		if (gT > i * 8 + 5) aiKarts[i]->setStarted(true);

	//Update Kart and Scene Physics - Physics is updated multiple times per frame to avoid jitter
	for (int i = 0; i < numPXUpdates; i++) {
		//for (int i = 0; i < numKartPXUpdates; i++) //update Karts numKartPXUpdates times for each rendered frame
		for (int i = 0; i < numKartPXUpdates; i++) //update Karts numKartPXUpdates times for each rendered frame
			PXKartController->stepVehicles(stepSize * 1.6f / (numPXUpdates * numKartPXUpdates), kartArray, NUM_VEHICLES);
		for (int i = 0; i < numScenePXUpdates; i++) //update PhysX Scene numScenePXUpdates times for each rendered frame
			PXScene->step(stepSize * 1.6f / (numPXUpdates * numScenePXUpdates));
	}

	//Todo  - Update Karts transforms from network here

	//Update Karts Graphics
	playerKart->update(context, stepSize);

	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		aiKarts[i]->update(context, stepSize);


	

	// update camera
	if (camera->getFlying())//if flying 
	{
		XMFLOAT3 velovityFloat3 = m_controller->Velocity();
		XMVECTOR velocityVec = XMLoadFloat3(&velovityFloat3);
		camera->setPos(XMVectorAdd(camera->getPos(), DirectX::XMVectorScale(velocityVec, timer.GetElapsedSeconds() * 5.0f)));
		XMFLOAT3 lookDirFloat3 = m_controller->LookDirection();
		XMVECTOR lookDirVec = XMLoadFloat3(&lookDirFloat3);
		//if (!camera->getFlying())//if not flying stick to ground
		//{
		//	XMVECTOR camPos = camera->getPos();
		//	camPos.m128_f32[1] = 1 + ground->CalculateYValueWorld(camPos.m128_f32[0], camPos.m128_f32[2]);
		//	camera->setPos(camPos);
		//}
		camera->setLookAt(DirectX::XMVectorAdd(camera->getPos(), lookDirVec));
		camera->update(context);
	}
	else
	{
		//Update Player Camera
		physx::PxTransform trans = playerKart->getVehicle4W()->getRigidDynamicActor()->getGlobalPose();
		XMVECTOR pos = XMLoadFloat4(&XMFLOAT4(trans.p.x, trans.p.y + 0.05, trans.p.z, 1.0));
		if (playerKart->camMode == 3)//top down view
		{
			//Top Down View
			camera->updateTopDown(context, pos, 15.0);
		}
		else
		{
			XMVECTOR quart = XMLoadFloat4(&XMFLOAT4(trans.q.x, trans.q.y, trans.q.z, trans.q.w));
			float timeScale = stepSize * 60.0f;
			//First Person
			//camMode == 0
			XMVECTOR desiredCamOffset = XMVectorSet(0.0, 1.0, -0.6, 1.0);
			XMVECTOR desiredLookAtOffset = XMVectorSet(0.0, 1.0, 6.0, 1.0);
			float currentWeightDIR = 0.5 * timeScale;
			float currentWeightPOS = 0.7 * timeScale;
			if (playerKart->camMode == 1)
			{
				//Close Follow
				desiredCamOffset = XMVectorSet(0.0, 1.0, -2.0, 1.0);
				desiredLookAtOffset = XMVectorSet(0.0, 1.0, 0.0, 1.0);
				currentWeightDIR = 0.1 * timeScale;
				currentWeightPOS = 0.1 * timeScale;
			}
			else if (playerKart->camMode == 2)
			{
				//Far Follow
				desiredCamOffset = XMVectorSet(0.0, 1.0, -2.0, 1.0);
				desiredLookAtOffset = XMVectorSet(0.0, 1.0, 0.0, 1.0);
				currentWeightDIR = 0.05 * timeScale;
				currentWeightPOS = 0.05 * timeScale;
			}
			camera->updateFollow(context, pos, quart, currentWeightDIR, currentWeightPOS, desiredCamOffset, desiredLookAtOffset);
		}

	}

	float tod = sin(gT / 20.0f) / 2 + 0.5f;
	float todBlue = tod * 0.5 + 0.5;
	sceneLights->setLightAmbient(context,0,XMFLOAT4(0.3 * tod, 0.3 * tod, 0.3 * todBlue, 1.0));
	sceneLights->setLightDiffuse(context, 0, XMFLOAT4(0.8 * tod, 0.8 * tod, 1.0 * todBlue, 1.0));
	sceneLights->setLightSpecular(context, 0, XMFLOAT4(0.8 * tod, 0.8 * tod, 1.0 * todBlue, 1.0));












	float dragonHeight0 = 0;
	// Animate Dragons
	//if (NUM_DRAGONS > 0)
	//{
	//	// move to next animation sequence after TARGET seconds
	//	float TARGET = 25;//seconds
	//	double swapAnimTime[MAX_DRAGONS] = { 0,0,0,0,0,0,0,0 };
	//	static double nextAnimTimer[MAX_DRAGONS] = { 0,0,0,0,0,0,0,0 };

	//	for (int i = 0; i < NUM_DRAGONS; i++)
	//	{
	//		//dT = timer.GetElapsedSeconds();

	//		/*double dragondt = dT;*/

	//		double param = TARGET / dragon[i]->getAnimLength(), intpart;
	//		modf(param, &intpart);
	//		swapAnimTime[i] = (dragon[i]->getAnimLength()*intpart);
	//		//if (nextAnimTimer[i]<1.0 || nextAnimTimer[i]>swapAnimTime[i] - 1.0)
	//		//	dragondt = dT / 4.0;

	//		nextAnimTimer[i] += dT;

	//		if (nextAnimTimer[i] >= swapAnimTime[i])
	//		{
	//			nextAnimTimer[i] = 0.0;
	//			//dragondt = 0.0;
	//			/*for (int i = 0; i < NUM_DRAGONS; i++)*/
	//			if (dragon[i]->getCurrentAnim() == dragon[i]->getNumAnimimations() - 1)
	//				dragon[i]->setCurrentAnim(0);
	//			else
	//				dragon[i]->setCurrentAnim(dragon[i]->getCurrentAnim() + 1);
	//		}

	//		float r = 0;
	//		if (dragon[i]->getCurrentAnim() == 2)r = -0.4*dT;
	//		else if (dragon[i]->getCurrentAnim() == 3)r = -0.2*dT;
	//		dragon[i]->setWorldMatrix(dragon[i]->getWorldMatrix()*XMMatrixRotationY(r));
	//		XMVECTOR dragonPos = XMVectorZero();
	//		dragonPos = XMVector2TransformCoord(dragonPos, dragon[i]->getWorldMatrix());
	//		float dragonHeight = ground->CalculateYValueWorld(dragonPos.m128_f32[0], dragonPos.m128_f32[2]);
	//		if (i == 0)dragonHeight0 = dragonHeight;
	//		dragon[i]->setWorldMatrix(dragon[i]->getWorldMatrix()*XMMatrixTranslation(0, dragonHeight - dragonPos.m128_f32[1], 0));
	//	}
	//	if (dragon[0]->getCurrentAnim() == 1)
	//	{
	//		double currentAnimTime = fmod(gT, dragon[0]->getAnimDuration(dragon[0]->getCurrentAnim()));
	//		UpdateDragonFire(context, dragon[0]->getWorldMatrix(), dragonHeight0, nextAnimTimer[0]);
	//	}
	//	else
	//		fire->setVisible(false);

	//	fire->update(context);
	//	updateDragons(nextAnimTimer);//update bones in separate threads

	//	for (int i = 0; i < NUM_DRAGONS; i++)
	//		dragon[i]->update(context); //calls BaseModel update to update BaseModels WorldMatrix CBuffer

		////textBuffer += L"Elapsed Time = " + std::to_wstring(nextAnimTimer[0]) + L"Swap Time = " + std::to_wstring(swapAnimTime[0]) + L"Anim Time = " + std::to_wstring(dragon[0]->getAnimTime(nextAnimTimer[0])) + L"Anim Length = " + std::to_wstring(dragon[0]->getAnimLength());
	//}



}
void Scene::UpdateDragonFire(ID3D11DeviceContext *context, XMMATRIX dragonMat, float dragonHeight, double currentAnimTime)
{
	XMVECTOR pos, rot, scale;
	XMMatrixDecompose(&pos, &rot, &scale, dragonMat);
	//fire->setWorldMatrix(XMMatrixTranslation(10, 0, 0)*XMMatrixRotationQuaternion(rot));

	// dragonHeight = pos.vector4_f32[1];
	cout << "height" << dragonHeight << "pos" << pos.m128_f32[1] << endl;
	float t = 0, angle = 0, height = 0;
	float t_range = 0;
	float start_height = 0, end_height = 0;
	float start_angle = 0, end_angle = 0;

	if (currentAnimTime > 5.0 && currentAnimTime < 5.25)
	{
		t_range = (5.25 - 5.0);
		start_height = 2.4, end_height = 3.1;
		start_angle = 120.0, end_angle = 70.0;
		t = (currentAnimTime - 5.0);
	}
	else if (currentAnimTime > 5.25 && currentAnimTime < 5.65)
	{
		t_range = (5.65 - 5.25);
		start_height = 3.1, end_height = 3.1;
		start_angle = 70.0, end_angle = 60.0;
		t = (currentAnimTime - 5.25);
	}
	else if (currentAnimTime > 5.65&&currentAnimTime < 6.1)
	{
		t_range = (6.1 - 5.65);
		start_height = 3.1, end_height = 2.9;
		start_angle = 60.0, end_angle = 90.0;
		t = (currentAnimTime - 5.65);
	}
	else if (currentAnimTime > 6.1&&currentAnimTime < 6.25)
	{
		t_range = (6.25 - 6.1);
		start_height = 2.9, end_height = 2.7;
		start_angle = 90.0, end_angle = 90.0;
		t = (currentAnimTime - 6.1);
		
	}
	if (currentAnimTime > 5.0&&currentAnimTime < 6.25)
	{
		paused = true;
		float height_range = end_height - start_height;
		float angle_range = end_angle - start_angle;
		float lerp = (1.0f / t_range)*t;
		angle = start_angle + angle_range * lerp;
		height = start_height + height_range * lerp;
		//fire->setWorldMatrix(XMMatrixRotationX(XMConvertToRadians(angle - 30)) *XMMatrixTranslation(0.0, height + dragonHeight + 0.1, 1.0)*fire->getWorldMatrix());

		t = (currentAnimTime - 5);
		float lightFactor = (t) *(1.0f / 0.60);;
		if (t > 0.60) {
			t -= 0.60; lightFactor = (t) *(1.0f / 0.60); lightFactor = 1 - lightFactor;
		}

		XMVECTOR firepos = XMVectorZero();
		//firepos = XMVector3TransformCoord(firepos, fire->getWorldMatrix());

		//fire->setWorldMatrix(XMMatrixScaling(lightFactor, lightFactor, lightFactor)*fire->getWorldMatrix());
		//fire->update(context);
		//fire->setVisible(true);
		sceneLights->setLightAttenuation(context, 1, XMFLOAT4(0.1, 0.0, 0.90, 10.0));
		//sceneLights->setLightAttenuation(context,0, XMFLOAT4(0.1, 0.00, 0.60000, -1.0));
		sceneLights->setLightVector (context,1,XMFLOAT4(firepos.m128_f32[0], firepos.m128_f32[1], firepos.m128_f32[2], 1.0));
		//sceneLights->setLightVector(context, 1, XMFLOAT4(0, 0, 0, 1.0));
		sceneLights->setLightDiffuse(context,1,XMFLOAT4(6 * lightFactor, 3 * lightFactor, 0.0, 1.0));
		sceneLights->setLightAmbient(context, 1, XMFLOAT4(6 * lightFactor, 3 * lightFactor, 0.0, 1.0));
		sceneLights->setLightSpecular(context, 1, XMFLOAT4(6* lightFactor, 3 * lightFactor, 0.0, 1.0));
		sceneLights->update(context);
		//sceneLights->setLightDiffuse(context,1,XMFLOAT4(0.5*lightFactor, 0.25*lightFactor, 0.0, 1.0));
		//sceneLights->setLightSpecular(context,1,XMFLOAT4(0.3*lightFactor, 0.1*lightFactor, 0.0, 1.0));

		//cBufferLightCPU[1].lightVec = XMFLOAT4(firepos.vector4_f32[0], firepos.vector4_f32[1], firepos.vector4_f32[2], 1.0);
		//cBufferLightCPU[1].lightDiffuse = XMFLOAT4(0.5*lightFactor, 0.25*lightFactor, 0.0, 1.0);
		//cBufferLightCPU[1].lightSpecular = XMFLOAT4(0.3*lightFactor, 0.1*lightFactor, 0.0, 1.0);
		//mapCbuffer(context, cBufferLightCPU, cBufferLightGPU, sizeof(CBufferLight) * 2);
	}
	else
	{
		//fire->setVisible(false);
		sceneLights->setLightAttenuation(context, 1, XMFLOAT4(1.0, 0.00, 0.0000, -1.0));
		paused = false;
	}

	//fire->update(context);
}
typedef struct MyData {
	//SkinnedModel* dragon;
	double gT;
} MYDATA, *PMYDATA;

void Scene::updateDragons(double gT[])
{
	DWORD WINAPI DragonUpdateThread(LPVOID lpParam);

	// Sample custom data structure for threads to use.
	// This is passed by void pointer so it can be any data type
	// that can be passed using a single void pointer (LPVOID).	
	{
		PMYDATA pDataArray[MAX_DRAGONS];
		DWORD   dwThreadIdArray[MAX_DRAGONS];
		HANDLE  hThreadArray[MAX_DRAGONS];

		// Create MAX_THREADS worker threads.

		for (int i = 0; i<NUM_DRAGONS; i++)
		{
			// Allocate memory for thread data.
			pDataArray[i] = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MYDATA));

			// If the array allocation fails, terminate execution.
			if (pDataArray[i] == NULL)
				break;
		
			// Generate unique data for each thread to work with.
			//pDataArray[i]->dragon = dragon[i];
			pDataArray[i]->gT =gT[i];

			// Create the thread to begin execution on its own.
			hThreadArray[i] = CreateThread(NULL,0,DragonUpdateThread,pDataArray[i],0,&dwThreadIdArray[i]);   // returns the thread identifier 
			// Check the return value for success.If CreateThread fails, terminate execution. 
			
			if (hThreadArray[i] == NULL)
				break;
		} // End of main thread creation loop.

		// Wait until all threads have terminated.
		WaitForMultipleObjects(NUM_DRAGONS, hThreadArray, TRUE, INFINITE);

		// Close all thread handles and free memory allocations.
		for (int i = 0; i<NUM_DRAGONS; i++)
		{
			CloseHandle(hThreadArray[i]);
			if (pDataArray[i] != NULL)
			{
				HeapFree(GetProcessHeap(), 0, pDataArray[i]);
				pDataArray[i] = NULL;    // Ensure address is not reused.
			}
		}
	}
}

DWORD WINAPI DragonUpdateThread(LPVOID lpParam)
{
	PMYDATA pDataArray;
	// Cast the parameter to the correct data type.
	// The pointer is known to be valid because 
	// it was checked for NULL before the thread was created.
	pDataArray = (PMYDATA)lpParam;
	//pDataArray->dragon->updateBones(pDataArray->gT);
	return 0;
}
void Scene::DrawGrass(ID3D11DeviceContext *context)
{

		// Render grass layers from base to tip
		for (int i = 0; i < numGrassPasses; i++)
		{
			cBufferSceneCPU->grassHeight = (grassLength / numGrassPasses)*i;
			mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
			//ground->render(context);
		}
	
}

//
void Scene::RenderSceneObjects(ID3D11DeviceContext *context)
{
	ID3D11Device* device = m_deviceResources->GetD3DDevice();
//	ID3D11DeviceContext* context = m_deviceResources->GetD3DDeviceContext();
	// Render SkyBox
	//if (skyBox)
//		skyBox->render(context);

	// Render trees
	//if (true)
	//	for (int i = 0; i < numTrees; i++)
	//	{
	//		int type = treeTypeArray[i];
	//		trees[type]->setWorldMatrix(treeArray[i]);
	//		trees[type]->update(context);
	//		trees[type]->render(context);
	//	}

	if (skyBox)
		skyBox->render(context);



	////Render Dragons
	//for (int i = 0; i < NUM_DRAGONS; i++)
	//	if (dragon[i]->getVisible())
	//		dragon[i]->render(context);

	//Render ground
	//Render ground base shell solid
	cBufferSceneCPU->grassHeight = 0.0f;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	//grass->getEffect()->setDepthWriteMask(device, D3D11_DEPTH_WRITE_MASK_ALL);
	grass->render(context);
	//Render the racing line
	if (models.size() > 0)
		models[0]->renderInstances(context);






	//Render remaining grass shells with alphablending on but dont write to the depth buffer
	//grass->getEffect()->setAlphaBlendEnable(device, true);
	//grass->getEffect()->setDepthWriteMask(device, D3D11_DEPTH_WRITE_MASK_ZERO);

	// Render grass layers from base to tip
	for (int i = 1; i < numGrassPasses; i++)
	{
		cBufferSceneCPU->grassHeight = (grassLength / numGrassPasses) * i;
		mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
		grass->render(context);
	}

	//if (grass->getVisible())
	//{
	//	grass->render(context);


		////shadowMap->update(context);
		////shadowMap->setShadowMatrix(context);
		//// Render grass
//		if (ground)
	//		DrawGrass(context);
	//}
	//Render orb
	//if (orb)
	//	orb->render(context);
	//if (castle->getVisible())
	//	castle->render(context);


	////Render the karts
	//playerKart->render(context, playerKart->camMode);
	//for (int i = 0; i < NUM_AI_VEHICLES; i++)
	//	aiKarts[i]->render(context);

	renderShadowObjects(context);
}
void Scene::DrawFlare(ID3D11DeviceContext* context)
{
	// Draw the lense flares (Draw all transparent objects last)
	if (flares) {

		// Set NULL depth buffer so we can also use the Depth Buffer as a shader
		// resource this is OK as we don’t need depth testing for the flares
		ID3D11RenderTargetView* tempRT = m_deviceResources->GetBackBufferRenderTargetView();
		context->OMSetRenderTargets(1, &tempRT, NULL);
		ID3D11ShaderResourceView* depthSRV = m_deviceResources->GetDepthStencilSRV();
		context->VSSetShaderResources(5, 1, &depthSRV);
		flares->renderInstances(context);
		// Use Null SRV to release depth shader resource so it is available for writing
		ID3D11ShaderResourceView* nullSRV = NULL;
		context->VSSetShaderResources(5, 1, &nullSRV);
		// Return default (read and write) depth buffer view.
		tempRT = m_deviceResources->GetBackBufferRenderTargetView();
		context->OMSetRenderTargets(1, &tempRT, m_deviceResources->GetDepthStencilView());
	}
}
void Scene::renderShadowObjects(ID3D11DeviceContext* context)
{

//	if (castle->getVisible())
//		castle->render(context);
	//Render the karts
	playerKart->render(context, playerKart->camMode);
	for (int i = 0; i < NUM_AI_VEHICLES ; i++)
		aiKarts[i]->render(context);

	////Render all tree instances
	//if (tree)
	//	tree->renderInstances(context);

	//Render all loaded models
	for (int i = 1; i < models.size(); i++)
		models[i]->renderInstances(context);
	////Render the dragon
	//dragon->render(context);
	////Render the nathan
	//nathan->renderInstances(context);
	////Render the sophia
	//sophia->renderInstances(context);

}
// Renders one frame using the vertex and pixel shaders.
void Scene::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
		return;

	auto context = m_deviceResources->GetD3DDeviceContext();
	auto device = m_deviceResources->GetD3DDevice();

	
	
	
	if (MainMenuState < MenuDone)
	{
		//Show the mainMenu
		menu->renderMainMenu(playerKart, &MainMenuState, stepSize);
		return;
	}
	
	////Render to ShadowMap
	



			//Disable ShadowMap
	cBufferSceneCPU->USE_SHADOW_MAP = false;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	shadowMap->update(context);

	//Disable Alpha to Coverage as Shadow Map is not MultiSampled
	//tree->getEffect()->setAlphaToCoverage(device, FALSE);
	models[1]->getEffect()->setAlphaToCoverage(device, FALSE);

	//Render objects to shadow map
	shadowMap->render(m_deviceResources, std::bind(&Scene::renderShadowObjects, this, std::placeholders::_1));

	//Enable ShadowMap
	cBufferSceneCPU->USE_SHADOW_MAP = true;
	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));

	//Enable Alpha to Coverage
	//tree->getEffect()->setAlphaToCoverage(system->getDevice(), TRUE);
	models[1]->getEffect()->setAlphaToCoverage(device, TRUE);

	//dynamicCubeMap->updateCubeCameras(context, XMVectorSet(XMVectorGetX(mainCamera->getPos()), -XMVectorGetY(mainCamera->getPos()) + 1.8, XMVectorGetZ(mainCamera->getPos()), 1));
	//dynamicCubeMap->render(system, std::bind(&Scene::renderDynamicObjects, this, std::placeholders::_1));

	//The Cube Enviroment Texture is permenantly bound to slot 6 as it is used by a number of effects
	//ID3D11ShaderResourceView* cubeDayTextureSRV = cubeDayTexture->getShaderResourceView();
	//context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);
	//Return the main camera





	//ID3D11ShaderResourceView* nullSRV = nullptr;
	//context->PSSetShaderResources(2, 1, &nullSRV);
	//context->PSSetShaderResources(1, 1, &nullSRV);
	//context->PSSetShaderResources(4, 1, &nullSRV);
	//cBufferSceneCPU->USE_SHADOW_MAP = false;
	//ground->setVisible(false);

	//for (int i = 0; i < NUM_DRAGONS; i++)
	//	(dragon[i]->setVisible(true));
	////castle->setVisible(false);
	//shadowMap->update(context);
	//shadowMap->render(context, std::bind(&Scene::RenderSceneObjects, this, std::placeholders::_1));
	//cBufferSceneCPU->USE_SHADOW_MAP = true;
	//ground->setVisible(true);
	//castle->setVisible(true);
	//for (int i = 0; i < NUM_DRAGONS; i++)
	//	(dragon[i]->setVisible(true));
	//Restore main cameara
	camera->update(context);
	//Restore main viewport render target and depth stencil
	ID3D11DepthStencilView* depthStencil = m_deviceResources->GetDepthStencilView();
	ID3D11RenderTargetView* renderTargets[1];
	renderTargets[0] = m_deviceResources->GetBackBufferRenderTargetView();
	context->RSSetViewports(1, &(m_deviceResources->GetScreenViewport()));
	context->OMSetRenderTargets(1, renderTargets, depthStencil);


	 //Render Scene Objects
	RenderSceneObjects(context);



	if (sin(cBufferSceneCPU->Time / 20.0f) > 0.5)
		DrawFlare(context);

	//if (fire->getVisible())
	//{
	//	fire->update(context);
	//	fire->updateParticles(context);
	//	fire->render(context);
	//	//firstPass = false;
	//}
	////
	//// Render water
	//if (water)
	//	water->render(context);
}

void Scene::CreateDeviceDependentResources()
{
	ID3D11Device *device=m_deviceResources->GetD3DDevice();
	ID3D11DeviceContext *context = m_deviceResources->GetD3DDeviceContext();



	//Draw the intro screen while we are loading
	if (MainMenuState == Intro)
	{
		menu = new Menu(m_deviceResources);
		menu->init(&netMgr);
		//menu->renderIntro();
		//MainMenuState = MenuDone;
		MainMenuState = MainMenu;
	}


	//Init FmodStudio
	FMOD_RESULT result = FMOD::System_Create(&FMSystem);
	if (result != FMOD_OK)// Handle error
		cout << "cant create sound system" << endl;
	result = FMSystem->init(32, FMOD_INIT_NORMAL, nullptr);
	if (result != FMOD_OK)// Handle error
		cout << "cant init sound" << endl;




	
	
	//Initialise PhysX
	PXScene = new PhysXKarting();




	//Create a Shadow map
	XMVECTOR lightVec = XMVectorSet(-124.0, 50, 60, 1.0);
	shadowMap = new ShadowMap(device, lightVec, 4096);// 2048);//4096);// 8192);





	//shared_ptr<Material>skinMat(new Material(device));
	//skinMat->setSpecular(XMFLOAT4(1.0, 1.0, 1.0, 1.0));
	//skinMat->setTextures(faceTextureArray, 3);

	shared_ptr<Material>whiteMatt(new Material(device));
	whiteMatt->setSpecular(XMFLOAT4(0.0, 0.0, 0.0, 0.0));
	whiteMatt->setDiffuse(XMFLOAT4(1.0, 1.0, 1.0, 1.0));
	whiteMatt->setUsage(DIFFUSE_MAP);
	//whiteMatt->setTexture(Texture(device, L"..\\..\\..\\..\\..\\Resources\\Models\\Castle\\Castle.png").getShaderResourceView());
	//whiteMatt->setTexture(Texture(device, L"Assets\\Textures\\castle.jpg").getShaderResourceView());
	 whiteMatt->setTexture(Texture(device, ResourcePath + L"Resources\\Models\\Castle\\Castle.png").getShaderResourceView());
	//Material *whiteMattArray[] = { whiteMatt };
	//Material *whiteGloss = new Material();
	//whiteGloss->setSpecular(XMFLOAT4(1.0, 1.0, 1.0, 10.0));
	//whiteGloss->setDiffuse(XMFLOAT4(1.0, 1.0, 1.0, 1.0));
	//Material *whiteGlossArray[] = { whiteGloss };

	//Setup Effects and custom pipeline statesp										
	//Effect *skyBoxEffect = new Effect(device, "sky_box_vs.cso", "sky_box_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	//Effect *grassEffect = new Effect(device, "grass_vs.cso", "grass_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	//Effect *reflectionMapEffect = new Effect(device, "reflection_map_vs.cso", "reflection_map_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));		
	//Effect *waterEffect = new Effect(device, "ocean_vs.cso", "ocean_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	//Effect *ppLightEffect = new Effect(device, "per_pixel_lighting_vs.cso", "per_pixel_lighting_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));
	shared_ptr<Effect> perPixelLightingEffect(new Effect(device, "per_pixel_lighting_vs.cso", "per_pixel_lighting_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc)));
	//Setup SkyBox Effect
	shared_ptr<Effect>  skyBoxEffect(new Effect(device, "sky_box_vs.cso", "sky_box_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc)));
	skyBoxEffect->setCullMode(device, D3D11_CULL_FRONT);
	skyBoxEffect->setDepthFunction(device, D3D11_COMPARISON_LESS_EQUAL);

	//Setup Cube Enviroment Texture
	//TexturecubeNightTexture = new Texture(device, L"Resources\\Textures\\nightenvmap1024.dds");
	cubeDayTexture = new Texture(device, ResourcePath + L"Resources\\Textures\\grassenvmap1024.dds");
	ID3D11ShaderResourceView* cubeDayTextureSRV = cubeDayTexture->getShaderResourceView();
	//The Cube Enviroment Texture is permenantly bound to slot 6 as it is used by a number of effects
	context->PSSetShaderResources(6, 1, &cubeDayTextureSRV);
	// Create a skybox
// The box class is derived from the BaseModel class 
	shared_ptr<Material> skyBoxMaterial(new Material(device));
	skyBoxMaterial->setTexture(Texture(device, ResourcePath + L"Resources\\Textures\\nightenvmap1024.dds").getShaderResourceView());
	skyBox = new Box(device, skyBoxEffect, skyBoxMaterial);
	skyBox->setWorldMatrix(skyBox->getWorldMatrix() * XMMatrixScaling(1000, 1000, 1000));
	skyBox->update(context);

	//Effect *fireUpdateEffect = new Effect(device, "fire_vs.cso", "fire_ps.cso", "fire_update_gs.cso", particleVertexDesc, ARRAYSIZE(particleVertexDesc), particleSODesc, ARRAYSIZE(particleSODesc));
	//Effect *fireRenderEffect = new Effect(device, "fire_vs.cso", "fire_ps.cso", "fire_gs.cso", particleVertexDesc, ARRAYSIZE(particleVertexDesc));
	//Effect* shadowMapEffect = new Effect(device, "shadow_map_vs.cso", "pointlight_shadow_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc));

//	// The Effect class constructor sets default depth/stencil, rasteriser and blend states
//// The Effect binds these states to the pipeline whenever an object using the effect is rendered
//// We can customise states if required
//	ID3D11BlendState *partBS = fireRenderEffect->getBlendState();
//	D3D11_BLEND_DESC partBD; partBS->GetDesc(&partBD);
//
//	// Modify blend description - alpha blending on
//	partBD.RenderTarget[0].BlendEnable = true;
//	partBD.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
//	partBD.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
//	partBD.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;


	//// Create new blendState
	//device->CreateBlendState(&partBD, &partBS);
	//fireRenderEffect->setBlendState(partBS);

	//// get current depthStencil State and depthStencil description of particleEffect (depth read and write by default)
	//ID3D11DepthStencilState *partDSS = fireRenderEffect->getDepthStencilState();
	//D3D11_DEPTH_STENCIL_DESC partDSD; partDSS->GetDesc(&partDSD);
	////Disable Depth Writing for particles
	//partDSD.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	//// Create custom fire depth-stencil state object
	//device->CreateDepthStencilState(&partDSD, &partDSS);
	//fireRenderEffect->setDepthStencilState(partDSS);


	//ID3D11DepthStencilState *skyBoxDSState = skyBoxEffect->getDepthStencilState();
	//D3D11_DEPTH_STENCIL_DESC skyBoxDSDesc;skyBoxDSState->GetDesc(&skyBoxDSDesc);
	//skyBoxDSDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	//device->CreateDepthStencilState(&skyBoxDSDesc, &skyBoxDSState);
	//skyBoxEffect->setDepthStencilState(skyBoxDSState);

	//ID3D11RasterizerState *rsStateSky = skyBoxEffect->getRasterizerState();;
	//D3D11_RASTERIZER_DESC	RSdesc;rsStateSky->GetDesc(&RSdesc);
	//RSdesc.CullMode = D3D11_CULL_FRONT;
	//device->CreateRasterizerState(&RSdesc, &rsStateSky);
	//skyBoxEffect->setRasterizerState(rsStateSky);

	////Customise waterEffect blend state
	//ID3D11BlendState *waterBlendState = waterEffect->getBlendState();
	//D3D11_BLEND_DESC	blendDesc;
	//waterBlendState->GetDesc(&blendDesc);//the effect is initialised with the default blend state

	//// Add Code Here (Enable Alpha Blending for water)
	//blendDesc.AlphaToCoverageEnable = FALSE; // Use pixel coverage info from rasteriser (default)
	//blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//// Create custom fire blend state object
	//waterBlendState->Release(); device->CreateBlendState(&blendDesc, &waterBlendState);
	//waterEffect->setBlendState(waterBlendState);

	//// Customise waterEffect depth stencil state
	//// (depth-stencil state is initialised to default in effect constructor)
	//// Get the default depth-stencil state object 
	//ID3D11DepthStencilState	*waterDSstate = waterEffect->getDepthStencilState();
	//D3D11_DEPTH_STENCIL_DESC	dsDesc;// Setup default depth-stencil descriptor
	// waterDSstate->GetDesc(&dsDesc);

	//// Add Code Here (Disable Depth Writing for water)
	//dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	//// Create custom water depth-stencil state object
	//waterDSstate->Release(); device->CreateDepthStencilState(&dsDesc, &waterDSstate);
	//waterEffect->setDepthStencilState(waterDSstate);

	//// Set up custome effect states for grass and tree effects
	//ID3D11BlendState *grassBlendState = grassEffect->getBlendState();
	//D3D11_BLEND_DESC	blendDesc;
	//grassBlendState->GetDesc(&blendDesc);
	//blendDesc.RenderTarget[0].BlendEnable = FALSE;
	//blendDesc.AlphaToCoverageEnable = TRUE; // Use pixel coverage info from rasteriser 
	//blendDesc.AlphaToCoverageEnable = FALSE; // Use pixel coverage info from rasteriser (default)
	//blendDesc.RenderTarget[0].BlendEnable = TRUE;
	//blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	//blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	//device->CreateBlendState(&blendDesc, &grassBlendState);
	//grassEffect->setBlendState(grassBlendState);
	//treeEffect->setBlendState(grassBlendState);



	//XMFLOAT4(-1250, 1000, 700, 1.0)
	//XMVECTOR lightVec = XMVectorSet(-62.0, 50, 35, 1.0); //-125.0, 100.0, 70.0, 1.0

	//shadowMap = new ShadowMap(device, shadowMapEffect, lightVec, 2048);//lightVec



	// Load Textures
	//Texture *sampleTexture = new Texture(device, L"Assets\\flag.jpg");
	//Texture* cubeDayTexture = new Texture(device, L"Assets\\Textures\\grassenvmap1024.dds");
	////Texture* waterNormalTexture = new Texture(device, L"Assets\\Textures\\Waves.dds");

	//Texture* grassAlpha = new Texture(device, L"Assets\\Textures\\grassAlpha.tif");
	//Texture* grassTexture = new Texture(device, L"Assets\\Textures\\grass1.bmp");
	//Texture* heightMap = new Texture(device, L"Assets\\Textures\\heightmap2.bmp");
	//Texture* normalMap = new Texture(device, L"Assets\\Textures\\normalmap.bmp");
	//Texture* grassNormals = new Texture(device, L"Assets\\Textures\\GroundNormalMap.png");
	////Texture* castleMap = new Texture(device, L"Assets\\Textures\\castle.jpg");
	//Texture *fireTexture = new Texture(device, L"Assets\\Textures\\Fire.tif");
	//ID3D11ShaderResourceView *cubeMap[] = { cubeDayTexture->getShaderResourceView() };
	//ID3D11ShaderResourceView *skyBoxTextureArray[] = { cubeDayTexture->getShaderResourceView() };
////	ID3D11ShaderResourceView *grassTextureArray[] = { grassTexture->getShaderResourceView(), grassAlpha->getShaderResourceView(), grassNormals->getShaderResourceView(),cubeDayTexture->getShaderResourceView(),shadowMap->getSRV()  };
	//ID3D11ShaderResourceView *grassTextureArray[] = { grassTexture->getShaderResourceView(), grassAlpha->getShaderResourceView(), grassNormals->getShaderResourceView(),cubeDayTexture->getShaderResourceView() };

//ID3D11ShaderResourceView *orbTextureArray[] = {nullptr, cubeDayTexture->getShaderResourceView() };
	//ID3D11ShaderResourceView *waterTextureArray[] = { waterNormalTexture->getShaderResourceView(), cubeDayTexture->getShaderResourceView() };

	//ID3D11ShaderResourceView *castleMapArray[] = { castleMap->getShaderResourceView(),nullptr,nullptr,nullptr,shadowMap->getSRV() };
	//ID3D11ShaderResourceView *fireTextureArray[] = { fireTexture->getShaderResourceView() };

	// Setup Objects - the object below are derived from the Base model class
	// The constructors for all objects derived from BaseModel require at least a valid pointer to the main DirectX device
	// And a valid effect with a vertex shader input structure that matches the object vertex structure.
	// In addition to the Texture array pointer mentioned above an additional optional parameters of the BaseModel class are a pointer to an array of Materials along with an int that contains the number of Materials
	// The baseModel class now manages a CBuffer containing model/world matrix properties. It has methods to update the cbuffers if the model/world changes 
	// The render methods of the objects attatch the world/model Cbuffers to the pipeline at slot b0 for vertex and pixel shaders

	// Create a skybox
	// The box class is derived from the BaseModel class 
	//skyBox = new Box(device, skyBoxEffect, NULL, 0, skyBoxTextureArray, 1);
	//// Add code here scale the box x1000
	//skyBox->setWorldMatrix(skyBox->getWorldMatrix()*XMMatrixScaling(100000, 100000, 100000));
	//skyBox->update(context);


	////Create grassy ground terrain
	//ground = new Terrain(device,context,400,400, heightMap->getTexture(), normalMap->getTexture(),  grassEffect, whiteMattArray, 1, grassTextureArray, 4);
	//ground->setWorldMatrix(ground->getWorldMatrix()*XMMatrixScaling(0.5, 2.5, 0.5)*XMMatrixTranslation(-80, -0.25, -100)*XMMatrixRotationY(XMConvertToRadians(45)));
	////ground->setWorldMatrix(ground->getWorldMatrix()*XMMatrixScaling(0.5, 0.5, 0.5));// *XMMatrixTranslation(-80, -0.25, -100)*XMMatrixRotationY(XMConvertToRadians(45)));
	//ground->update(context);



	//orb3 = new Model(context, device, wstring(L"..\\Resources\\Models\\sphere.obj"), beeEffect, beeMat, 0);
	//XMMATRIX beeInstanceMat = XMMatrixRotationZ(XMConvertToRadians(90)) * XMMatrixRotationY(XMConvertToRadians(-90)) * XMMatrixScaling(0.05, 0.05, 0.05) * XMMatrixTranslation(4.13, grass->CalculateYValueWorld(4.13, 58.30) + 1.12, 58.30);
	//XMMATRIX beeInstanceMat = XMMatrixScaling(0.025, 0.025, 0.025) * XMMatrixTranslation(4.13, 0, 58.30);

		// Create grassy ground terrain
	shared_ptr<Effect> grassEffect(new Effect(device, "grass_vs.cso", "grass_ps.cso", extVertexDesc, ARRAYSIZE(extVertexDesc)));

	// Set up custome effect states for grass and tree effects
ID3D11BlendState *grassBlendState = grassEffect->getBlendState();
D3D11_BLEND_DESC	blendDesc;
grassBlendState->GetDesc(&blendDesc);
blendDesc.RenderTarget[0].BlendEnable = FALSE;
blendDesc.AlphaToCoverageEnable = TRUE; // Use pixel coverage info from rasteriser 
blendDesc.AlphaToCoverageEnable = FALSE; // Use pixel coverage info from rasteriser (default)
blendDesc.RenderTarget[0].BlendEnable = TRUE;
blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
device->CreateBlendState(&blendDesc, &grassBlendState);
grassEffect->setBlendState(grassBlendState);


	// Alpha Blending On
	grassEffect->setAlphaBlendEnable(device, TRUE);
	//Load grass textures
	Texture grassAlpha = Texture(device, ResourcePath+ L"Resources\\Textures\\grassAlpha.tif");
	Texture grassDiffuse = Texture(device, ResourcePath + L"Resources\\Textures\\BrightonKarting5.bmp");
	Texture grassColour = Texture(device, ResourcePath + L"Resources\\Textures\\grass_texture.dds");
	Texture groundNormals = Texture(device, ResourcePath + L"Resources\\Textures\\Seamless_Asphalt_Texture_NORMAL.jpg");
	Texture asphaltDiffuse = Texture(device, ResourcePath + L"Resources\\Textures\\asphalt.dds");
	Texture heightMap = Texture(device, ResourcePath + L"Resources\\Levels\\Terrain01.bmp");
	Texture noiseMap = Texture(device, ResourcePath + L"Resources\\Levels\\noise.bmp");
	ID3D11ShaderResourceView* noise = noiseMap.getShaderResourceView();
	context->PSSetShaderResources(8, 1, &noise);
	context->VSSetShaderResources(8, 1, &noise);
	ID3D11ShaderResourceView* grassTextureArray[] = { grassDiffuse.getShaderResourceView(),asphaltDiffuse.getShaderResourceView(), grassAlpha.getShaderResourceView(),grassColour.getShaderResourceView(),groundNormals.getShaderResourceView(), noiseMap.getShaderResourceView() };
	shared_ptr<Material> grassMaterial(new Material(device));
	grassMaterial->setTextures(grassTextureArray, 6);
	//Create the grassy terrain
	float terrainOffsetXZ = -(terrainSizeWH * terrainScaleXZ) / 2.0f;
	Terrain *g2 = new Terrain(device, context, terrainSizeWH , terrainSizeWH , heightMap.getTexture(), grassEffect, grassMaterial);
	g2->setWorldMatrix(XMMatrixScaling(terrainScaleXZ, terrainScaleY, terrainScaleXZ)* XMMatrixTranslation(terrainOffsetXZ, -0.085, terrainOffsetXZ));
	g2->update(context);

	//grass = new Terrain(device, context, terrainSizeWH/2, terrainSizeWH/2, heightMap.getTexture(), grassEffect, grassMaterial);
	//grass->setWorldMatrix(XMMatrixScaling(terrainScaleXZ*2, terrainScaleY, terrainScaleXZ*2)* XMMatrixTranslation(terrainOffsetXZ, -0.085, terrainOffsetXZ));
	grass = new Terrain(device, context, terrainSizeWH , terrainSizeWH , heightMap.getTexture(), grassEffect, grassMaterial);
	grass->setWorldMatrix(XMMatrixScaling(terrainScaleXZ , terrainScaleY, terrainScaleXZ )* XMMatrixTranslation(terrainOffsetXZ, -0.085, terrainOffsetXZ));
	grass->update(context);
	grass->setColourMap(device, context, grassDiffuse.getTexture());


	XMFLOAT3 castlePos = XMFLOAT3(-1,0,0);
////	dragonPos = XMVector2TransformCoord(dragonPos, dragon[i]->getWorldMatrix());
	castle = new  Model(context,device, std::wstring(ResourcePath + L"Resources\\Models\\Castle\\Castle.obj"), perPixelLightingEffect, whiteMatt);
	//castle = new  Model(device, std::wstring(L"Assets\\Models\\castle.3ds"), ppLightEffect, whiteMattArray, 1, castleMapArray, 2);

	castlePos.y = grass->CalculateYValueWorld(castlePos.x, castlePos.z);
	castle->setWorldMatrix(XMMatrixScaling(1, 1, 1)* XMMatrixRotationX(XMConvertToRadians(-90))*XMMatrixTranslation(castlePos.x, castlePos.y, castlePos.z));
	castle->update(context);
//
//	fire = new GPUParticles(device, fireRenderEffect, fireUpdateEffect, whiteMattArray, 1, fireTextureArray, 1);
//	//fire->setWorldMatrix(XMMatrixIdentity());//   
//	fire->setWorldMatrix(XMMatrixRotationX(XMConvertToRadians(80)) *XMMatrixTranslation(0, 3.0, 0.7));
//	fire->update(context);
//
//	orb = new  Model(device, std::wstring( L"Assets\\Models\\sphere.3ds"), reflectionMapEffect, whiteGlossArray, 1, orbTextureArray, 2);
//	water = new Grid(device, 100, 100, waterEffect, whiteMattArray, 1, waterTextureArray, 2);
//	water->setWorldMatrix(water->getWorldMatrix()*XMMatrixTranslation(-40, -0, -50));
//	water->update(context);
//
//	// Add skinned dragons
//	Effect *skinningEffect = new Effect(device, "skinning_vs.cso", "per_pixel_lighting_ps.cso", skinVertexDesc, ARRAYSIZE(skinVertexDesc));
//	//Texture *greenDragonTexture = new Texture(device,context, L"Assets\\Textures\\Dragon_Bump_Col2.jpg");
//	Texture *dragonNormalTexture = new Texture(device, context, L"Assets\\Textures\\Dragon_Nor_mirror2.jpg");
//	Texture *greenDragonTexture = new Texture(device, context, L"Assets\\Textures\\Dragon_ground_color.jpg");
//	//Texture *dragonNormalTexture = new Texture(device, context, L"Assets\\Textures\\Dragon_Nor.jpg");
//
//	context->GenerateMips(greenDragonTexture->getShaderResourceView());
//	context->GenerateMips(dragonNormalTexture->getShaderResourceView());
//	//context->GenerateMips(greenDragonTexture2->getShaderResourceView());
//	//context->GenerateMips(dragonNormalTexture2->getShaderResourceView());
//	ID3D11ShaderResourceView *greenDragonTextureArray[] = { greenDragonTexture->getShaderResourceView(),dragonNormalTexture->getShaderResourceView() ,nullptr,nullptr,shadowMap->getSRV() };
//	
//
//	for (int i = 0; i < NUM_DRAGONS; i++)
//	{
//		Material *dragonMat = new Material(); dragonMat->setSpecular(XMCOLOR(0.2, 0.2, 0.2, 0.02)); dragonMat->setDiffuse(XMCOLOR(1.0, 1.0, 1.0, 1.0));
//		Material *dragonMatArray[] = { dragonMat };
//
//		dragon[i] = new  SkinnedModel(device, wstring(L"Assets\\Models\\Dragon_Baked_Actions_fbx_7_4_binary.fbxX"), skinningEffect, dragonMatArray, 1, greenDragonTextureArray, 5);
//		dragon[i]->setWorldMatrix(XMMatrixScaling(0.001, 0.001, 0.001)*XMMatrixTranslation(10, 0, 0)*XMMatrixRotationY(XMConvertToRadians(80*i)));
//		dragon[i]->setCurrentAnim(1);
//		dragon[i]->updateBones(i);
//		dragon[i]->setDiffuseTextCoords(1);
//		dragon[i]->setNormalTextCoords(0);
//		dragon[i]->update(context);
//	}


		//Create Lense Flares
	shared_ptr<Effect>  flareEffect(new Effect(device, "flare_vs.cso", "flare_ps.cso", flareVertexDesc, ARRAYSIZE(flareVertexDesc)));
	// Create custom flare blend state object
	ID3D11BlendState* flareBlendState = flareEffect->getBlendState();
	//D3D11_BLEND_DESC blendDesc;
	flareBlendState->GetDesc(&blendDesc);
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	flareBlendState->Release();
	device->CreateBlendState(&blendDesc, &flareBlendState);
	flareEffect->setBlendState(flareBlendState);
	//Load Flare Textures
	Texture flare0Texture = Texture(device, ResourcePath+ L"Resources\\Textures\\flares\\corona.png");
	Texture flare1Texture = Texture(device, ResourcePath + L"Resources\\Textures\\flares\\divine.png");
	Texture flare2Texture = Texture(device, ResourcePath + L"Resources\\Textures\\flares\\extendring.png");
	shared_ptr<Material> flareMat0(new Material(device, XMFLOAT4(1, 1, 1, (float)0 / numFlares)));
	flareMat0->setTexture(flare0Texture.getShaderResourceView());

	// Create Flares
	flares = new Flare(XMFLOAT3(-1250.0, 520.0, 1000.0), device, flareEffect, flareMat0);

	for (int i = 1; i < numFlares; i++)
	{
		shared_ptr<Material> flareInstanceMat(new Material(device, XMFLOAT4(randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, randM1P1() * 0.5 + 0.5, (float)i / numFlares)));
		if (randM1P1() > 0.0f)
			flareInstanceMat->setTexture(flare1Texture.getShaderResourceView());
		else
			flareInstanceMat->setTexture(flare2Texture.getShaderResourceView());
		flares->instances.push_back(Instance(XMMatrixIdentity(), flareInstanceMat));
	}


	//Init PhysX Karts scene
	PXScene->initScenePX();

	//Load models from Scene.xml created with the LevelEditor from ToolDev
	//We need to initialise PXScene first as some models have physical properties 
	load(WStringToString(ResourcePath) + string("Resources\\Levels\\Scene01_RH_X2.xml"), perPixelLightingEffect);

	//Create PhysX Terrain
	PXScene->CreateHeightField(g2->getHeightArray(), terrainSizeWH, terrainScaleXZ, terrainScaleY);


	//Load Karts







	//Load Kart wrap textures - read from XML file
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
		shared_ptr<Texture> tmpTex(new Texture(device, ResourcePath+ StringToWString(string("Resources\\Models\\Kart\\" + kartWraps[i]))));
		kartTextures.push_back(tmpTex);
	}

	//Load AI Kart NavPoints from XML file created using the LevelEditor from ToolDev
	navPoints = new NavPoints();
	navPoints->load(WStringToString(ResourcePath) + string("Resources\\Levels\\NavSet01.xml"));

	//Create the Karts and aply random wraps
	for (int i = 0; i < NUM_AI_VEHICLES - 1; i++)
		aiKarts[i] = new AIKart(FMSystem, device, context, grass, navPoints, perPixelLightingEffect, kartTextures[(int)(rand021() * (float)kartTextures.size())]);
	aiKarts[NUM_AI_VEHICLES - 1] = new AIKart(FMSystem, device, context, grass, navPoints, perPixelLightingEffect, kartTextures[(int)(rand021() * (float)kartTextures.size())]);
	int playerKartWrap = (int)(rand021() * (float)kartTextures.size());
	playerKart = new PlayerKart(FMSystem, device, context, grass, perPixelLightingEffect, kartTextures[playerKartWrap]);
	playerKart->m_controller = m_controller;
	//menu->setPlayerKartTextures(&kartWraps, &kartTextures, playerKartWrap);
	//Assign the Karts to an array for batch update by PhysX
	kartArray[0] = playerKart;
	for (int i = 0; i < NUM_AI_VEHICLES; i++)
		kartArray[i + 1] = aiKarts[i];
	physx::PxVec3 pP(36.0f, 0, 1.2f); pP.y = grass->CalculateYValueWorld(pP.x, pP.z) + pP.y;

	//float zpos = 1.2f;
	PXKartController = new PhysXVehicleController();
	playerKart->setVehicle4W(PXKartController->initVehiclePX(pP.x, pP.y, pP.z, -100.0f, 0));
	playerKart->setStartPosition(pP, 0);

	for (int i = 0; i < NUM_AI_VEHICLES; i++)
	{
		aiKarts[i]->setVehicle4W(PXKartController->initVehiclePX(pP.x + floor((i + 1) / 2) * 2, pP.y, pP.z - 2.0f * ((i + 1) % 2), -100.0f, i + 1));
		aiKarts[i]->setStartPosition(pP, i + 1);
	}






	// Add a Cbuffer to store world/scene properties
	// Allocate 16 byte aligned block of memory for "main memory" copy of cBufferScene
	cBufferSceneCPU = (CBufferScene *)_aligned_malloc(sizeof(CBufferScene), 16);
	D3D11_BUFFER_DESC cbufferDesc;
	D3D11_SUBRESOURCE_DATA cbufferInitData;
	ZeroMemory(&cbufferDesc, sizeof(D3D11_BUFFER_DESC));
	ZeroMemory(&cbufferInitData, sizeof(D3D11_SUBRESOURCE_DATA));
	cbufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbufferDesc.ByteWidth = sizeof(CBufferScene);
	
	// Fill out cBufferSceneCPU
	cBufferSceneCPU->windDir = XMFLOAT4(1, 0, 0, 1);
	cBufferSceneCPU->Time = 0.0;
	cBufferSceneCPU->grassHeight = 0.0;
	cBufferSceneCPU->USE_SHADOW_MAP = true;
	cBufferSceneCPU->numLights = 1;
	cBufferSceneCPU->grassHeight = 0.2;
	cBufferSceneCPU->fog = 0.15f;
	cBufferSceneCPU->QUALITY = 3;
	//cBufferSceneCPU->deltaTime = 0.0;
	//cBufferSceneCPU->REFLECTION_PASS = false;

	cbufferInitData.pSysMem = cBufferSceneCPU;// Initialise GPU CBuffer with data from CPU CBuffer

	device->CreateBuffer(&cbufferDesc, &cbufferInitData, &cBufferSceneGPU);

	mapCbuffer(context, cBufferSceneCPU, cBufferSceneGPU, sizeof(CBufferScene));
	context->VSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the vertex shader. 
	context->PSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the Pixel shader
	context->GSSetConstantBuffers(3, 1, &cBufferSceneGPU);// Attach CBufferSceneGPU to register b3 for the Geometry shader

	//light = new Light(device,XMFLOAT4(-1250, 1000, 700, 1.0),XMFLOAT4(0.4, 0.4, 0.4, 1.0),XMFLOAT4(1.0, 1.0, 1.0, 1.0),XMFLOAT4(1.0, 1.0, 1.0, 1.0),XMFLOAT4(1.0, 0.00, 0.0000, 10000.0));//const = 0.0; linear = 0.0; quad = 0.05; cutOff = 200.0;
	//CBufferLight lights[2] = { XMFLOAT4(-1250, 1000, 700, 1.0),XMFLOAT4(0.4, 0.4, 0.4, 1.0),XMFLOAT4(1.0, 1.0, 1.0, 1.0),XMFLOAT4(1.0, 1.0, 1.0, 1.0),XMFLOAT4(1.0, 0.00, 0.0000, 10000.0),
	//XMFLOAT4(20, 20, 20, 1.0),XMFLOAT4(0.1, 0.1, 0.0, 1.0),XMFLOAT4(1.0, 1.0, 0.0, 0.0),XMFLOAT4(1.0, 1.0, 0.0, 1.0),XMFLOAT4(1.0, 0.00, 0.0000, 100.0) };

	CBufferLight lights[1] = { XMFLOAT4(-1250, 1000, 700, 1.0),XMFLOAT4(0.4, 0.4, 0.4, 1.0),XMFLOAT4(1.0, 1.0, 1.0, 1.0),XMFLOAT4(1.0, 1.0, 1.0, 1.0),XMFLOAT4(1.0, 0.00, 0.0000, 10000.0) };
	lights->lightVec = XMFLOAT4(-1250.0, 1000.0, 5.0, 1.0);
	lights->lightAmbient = XMFLOAT4(0.3, 0.3, 0.5, 1.0);
	lights->lightDiffuse = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
	lights->lightSpecular = XMFLOAT4(0.9, 0.9, 0.9, 1.0);
	lights[0].lightAttenuation = XMFLOAT4(1.0, 0.0, 0.0, 10000.0);
	lights[0].lightCone = XMFLOAT4(0.0, -1.0, 0.0, 0);
	sceneLights = new LightArray(device, lights,1);//const = 0.0; linear = 0.0; quad = 0.05; cutOff = 200.0;

	sceneLights->update(context);

	//orb->setWorldMatrix(XMMatrixScaling(1,1,1)*XMMatrixTranslation(-1250, 1000, 700));
	//orb->update(context);
	m_controller->Active(true);
	m_loadingComplete = true;
	paused = false;
}
//Convert Model to PhysX Shape
void Scene::toPhysX(shared_ptr < Model> model, XMVECTOR R, XMVECTOR T, PhysXMode type, int instance)
{
	PxMaterial* material = mPhysics->createMaterial(0.0f, 0.0f, 0.6f);
	if (type == DRIVEABLE)
		material = mPhysics->createMaterial(0.6f, 0.4f, 0.6f);
	//Get Vertices
	if (type == STATIC || type == DRIVEABLE)//Static
	{
		PxVec3* verts = (PxVec3*)malloc(sizeof(PxVec3) * model->getNumVert());
		PxTransform t(PxVec3(0, 0, 0), PxQuat(XMConvertToRadians(0), PxVec3(0, 1, 0)));
		for (int i = 0; i < model->getNumVert(); i++)
		{
			XMVECTOR pos = DirectX::XMVector3TransformCoord(XMVectorSet(model->getVertexBufferCPU()[i].pos.x, model->getVertexBufferCPU()[i].pos.y, model->getVertexBufferCPU()[i].pos.z, 1), model->getWorldMatrix());
			verts[i].x = DirectX::XMVectorGetX(pos);
			verts[i].y = DirectX::XMVectorGetY(pos) - 0.25;
			verts[i].z = DirectX::XMVectorGetZ(pos);
		}
		int count = 0;
		//Get faces
		PxU32* indices32 = (PxU32*)malloc(sizeof(PxU32) * model->getNumInd());
		for (int j = 0; j < model->getNumMeshes(); j++)
			for (int i = 0; i < model->getIndexCount()[j]; i++) {
				indices32[count] = (PxU32)model->getIndexBufferCPU()[count] + model->getBaseVertexOffset()[j];
				count++;
			}
		//Create PhysX TriangleMesh
		PxTriangleMesh* triMesh = createTriangleMesh(verts, model->getNumVert(), indices32, model->getNumFaces(), *mPhysics, *gCooking);
		PxRigidStatic* r = mPhysics->createRigidStatic(t);
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*r, PxTriangleMeshGeometry(triMesh), *material);
		//Setup collision filters
		PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_CHASSIS, 0, 0);
		shape->setSimulationFilterData(simFilterData);

		if (type == DRIVEABLE)//make driveable
		{
			PxFilterData qryFilterData;
			setupDrivableSurface(qryFilterData);
			shape->setQueryFilterData(qryFilterData);
		}
		//Add the mesh to the PhysX Scene
		mScene->addActor(*r);
		free(verts);
		free(indices32);
	}
	else //Dynamic
	{
		PxTransform t(PxVec3(DirectX::XMVectorGetX(T), DirectX::XMVectorGetY(T) + 0.09, DirectX::XMVectorGetZ(T)), PxQuat(DirectX::XMVectorGetX(R), DirectX::XMVectorGetY(R), DirectX::XMVectorGetZ(R), DirectX::XMVectorGetW(R)));
		PxVec3* verts = (PxVec3*)malloc(sizeof(PxVec3) * model->getNumVert());

		for (int i = 0; i < model->getNumVert(); i++)
		{
			XMVECTOR pos = XMVectorSet(model->getVertexBufferCPU()[i].pos.x, model->getVertexBufferCPU()[i].pos.y, model->getVertexBufferCPU()[i].pos.z, 1);
			verts[i].x = DirectX::XMVectorGetX(pos);
			verts[i].y = DirectX::XMVectorGetY(pos);
			verts[i].z = DirectX::XMVectorGetZ(pos);
		}

		//Create PhysX ConvexMesh
		PxConvexMesh* convMesh = createConvexMesh(verts, model->getNumVert(), *mPhysics, *gCooking);
		PxRigidDynamic* r = mPhysics->createRigidDynamic(t);
		PxShape* shape = PxRigidActorExt::createExclusiveShape(*r, PxConvexMeshGeometry(convMesh), *material);
		//Setup collision filters
		PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE, 0, 0);
		shape->setSimulationFilterData(simFilterData);
		//Add the mesh to the PhysX Scene
		model->instances[instance].dynamicPX = r;

		mScene->addActor(*r);
		free(verts);
	}
}

//Load Models from XML file
void Scene::load(string path, shared_ptr<Effect> effect, float mapScale, float LHCoords)
{
	auto device = m_deviceResources->GetD3DDevice();

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

		if (model) //the scene has at lease 1 valid model so load it
			loadModel(model, &models, effect);
		if (models.size() > 1)//Model[0] is assumed to be the Racing Line, Model[1] is assumed to be be the boundry fence
		{
			//Give the racing line an emissive material so it glows
			models[0]->getMaterial(0)->setEmissive(XMFLOAT4(1, 1, 1, 1));

			//Racing line requires clamp type tileing of arrows default is mirror so we need a new sampler
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
			device->CreateSamplerState(&linearDesc, &sampler);
			models[0]->setSampler(sampler);

			//Fence needs to be drawn 2 sided and uses alpha to coverage
			shared_ptr<Effect>  effectA2C = make_shared<Effect>(effect);//safest method
			effectA2C->setCullMode(device, D3D11_CULL_NONE);
			effectA2C->setAlphaToCoverage(device, TRUE);
			models[1]->setEffect(effectA2C);
		}
	}
}

void Scene::loadModel(ti::XMLElement* model, vector< shared_ptr<Model>>* modelList, shared_ptr<Effect>  effect, float mapScale, float LHCoords)
{

	ID3D11DeviceContext* context = m_deviceResources->GetD3DDeviceContext();
	ID3D11Device* device = m_deviceResources->GetD3DDevice();

	while (model)
	{
		string path = model->FirstChildElement("PathName")->GetText();
		path = WStringToString(ResourcePath) + path.substr(6, path.length() - 6);
		string data = model->FirstChildElement("Data")->GetText();
		fs::path pathName("..\\"+path);
		shared_ptr<Material>mat(new Material(device));

		//load position rotation and scale from XML file;
		XMVECTOR pos = vecFromStr(data);
		pos = DirectX::XMVectorSetX(pos, DirectX::XMVectorGetX(pos) * mapScale);
		pos = DirectX::XMVectorSetZ(pos, DirectX::XMVectorGetZ(pos) * mapScale);
		pos = DirectX::XMVectorSetY(pos, grass->CalculateYValueWorld(DirectX::XMVectorGetX(pos), DirectX::XMVectorGetZ(pos)) + DirectX::XMVectorGetY(pos));
		XMVECTOR rot = vecFromStr(data);
		XMVECTOR scale = vecFromStr(data);
		scale = DirectX::XMVectorScale(scale, mapScale);
		XMMATRIX rotM = XMMatrixRotationX(DirectX::XMVectorGetX(rot)) * XMMatrixRotationY(DirectX::XMVectorGetY(rot)) * XMMatrixRotationZ(DirectX::XMVectorGetZ(rot));

		//get PhysX mode from XML file
		ti::XMLElement* physXEl = model->FirstChildElement("PhysX");
		string physX = "No"; if (physXEl)physX = physXEl->GetText();
		if (physX.compare("Dynamic") == 0)
		{
			//Dynamic models cannot be scaled so we need to pretransform the model vericies to remove the scale transform
			XMMATRIX PreTransScaleMat = XMMatrixScaling(DirectX::XMVectorGetX(scale), DirectX::XMVectorGetY(scale), DirectX::XMVectorGetZ(scale));
			shared_ptr<Model> mod(new Model(context, device, wstring(pathName.wstring().substr(3, path.length())), effect, nullptr, -1, &PreTransScaleMat));
			modelList->push_back(mod);
			(*modelList)[modelList->size() - 1]->setWorldMatrix(rotM * XMMatrixTranslation(DirectX::XMVectorGetX(pos), DirectX::XMVectorGetY(pos), DirectX::XMVectorGetZ(pos)));
			(*modelList)[modelList->size() - 1]->update(context);
			XMVECTOR Q = XMQuaternionRotationMatrix(rotM);
			//create a dynamic PhysX model and add it to PhysX scene
			toPhysX((*modelList)[modelList->size() - 1], Q, pos, DYNAMIC);
		}
		else
		{
			shared_ptr<Model> mod(new Model(context, device, wstring(pathName.wstring().substr(3, path.length())), effect, nullptr));
			//shared_ptr<Model> mod(new Model(context, device, pathName, effect, nullptr));
			//shared_ptr<Model> mod(new Model(context, device, L"Assets\\Resources\\Levels\\RacingLine_RH_X2.obj", effect, nullptr));
			modelList->push_back(mod);
			(*modelList)[modelList->size() - 1]->setWorldMatrix(rotM * XMMatrixScaling(DirectX::XMVectorGetX(scale), DirectX::XMVectorGetY(scale), DirectX::XMVectorGetZ(scale)) * XMMatrixTranslation(DirectX::XMVectorGetX(pos), DirectX::XMVectorGetY(pos), DirectX::XMVectorGetZ(pos)));
			(*modelList)[modelList->size() - 1]->update(context);
			if (physX.compare("Static") == 0)
				//create a static PhysX model and add it to PhysX scene
				toPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), STATIC);
			else if (physX.compare("Driveable") == 0)
				//create a static PhysX model and add it to PhysX scene
				toPhysX((*modelList)[modelList->size() - 1], XMVectorZero(), XMVectorZero(), DRIVEABLE);
		}
		cout << "Loaded:" << pathName.string() << endl;
		//Load instances if there are any
		ti::XMLElement* modelInstance = model->FirstChildElement("Instance");
		while (modelInstance)
		{
			data = modelInstance->FirstChildElement("Data")->GetText();
			XMVECTOR posI = vecFromStr(data);
			posI = DirectX::XMVectorSetX(posI, DirectX::XMVectorGetX(posI) * mapScale);
			posI = DirectX::XMVectorSetZ(posI, DirectX::XMVectorGetZ(posI) * mapScale);
			posI = DirectX::XMVectorSetY(posI, grass->CalculateYValueWorld(DirectX::XMVectorGetX(posI), DirectX::XMVectorGetZ(posI)) + DirectX::XMVectorGetY(posI));
			XMVECTOR rotI = vecFromStr(data);
			XMVECTOR scaleI = vecFromStr(data);
			scaleI = DirectX::XMVectorScale(scaleI, mapScale);
			XMMATRIX rotMI = XMMatrixRotationX(DirectX::XMVectorGetX(rotI)) * XMMatrixRotationY(DirectX::XMVectorGetY(rotI)) * XMMatrixRotationZ(DirectX::XMVectorGetZ(rotI));

			//Currently the PhysX properties of an instance are ignored and taken from the parent
			if (physX.compare("Dynamic") == 0)
			{
				XMVECTOR QI = XMQuaternionRotationMatrix(rotMI);
				(*modelList)[modelList->size() - 1]->instances.push_back(Instance(rotMI * XMMatrixTranslation(DirectX::XMVectorGetX(posI), DirectX::XMVectorGetY(posI), DirectX::XMVectorGetZ(posI)), (*modelList)[modelList->size() - 1]->getMaterials(0)));
				toPhysX((*modelList)[modelList->size() - 1], QI, posI, DYNAMIC, (*modelList)[modelList->size() - 1]->instances.size() - 1);
			}
			else
			{
				(*modelList)[modelList->size() - 1]->instances.push_back(Instance(rotMI * XMMatrixScaling(DirectX::XMVectorGetX(scaleI), DirectX::XMVectorGetY(scaleI), DirectX::XMVectorGetZ(scaleI)) * XMMatrixTranslation(DirectX::XMVectorGetX(posI), DirectX::XMVectorGetY(posI), DirectX::XMVectorGetZ(posI)), (*modelList)[modelList->size() - 1]->getMaterials(0)));
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

void Scene::ReleaseDeviceDependentResources()
{
	m_loadingComplete = false;

}