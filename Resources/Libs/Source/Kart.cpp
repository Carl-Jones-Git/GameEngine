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
#include <Kart.h>
#include <PhysXVehicleController.h>
extern wstring ResourcePath;

Kart::Kart(FMOD::System* _FMSystem,ID3D11Device* device, ID3D11DeviceContext* context, Terrain *_ground, shared_ptr<Effect>  _perPixelEffect,  shared_ptr<Texture> kartTexture)
{
	ground = _ground;
	mIsVehicleInAir = true;
	polePosition = physx::PxVec3(0, 0, 0);
	//Load Engine Sounds
	FMSystem = _FMSystem;
	FMOD_RESULT result;

	result = FMSystem->createSound(WStringToString(ResourcePath + L"Resources/Sounds/Two Stroke Revving2.mp3").c_str(), FMOD_DEFAULT | FMOD_LOOP_NORMAL, nullptr, &engine2);
	result = FMSystem->createSound(WStringToString(ResourcePath + L"Resources/Sounds/Two Stroke Revving3.mp3").c_str(), FMOD_DEFAULT | FMOD_LOOP_NORMAL, nullptr, &engine3);
	if (result != FMOD_OK)
		// Handle error
		cout << "cant find sound" << endl;
	else
		cout << "sound loaded" << endl;
	//Start playing engine sounds
	result = FMSystem->playSound(engine2, nullptr, true, &channel2);
	result = FMSystem->playSound(engine3, nullptr, true, &channel3);

	//Create double sides render Effect needed for seat as it is double sided
	shared_ptr<Effect> perPixelEffectCullNone(make_shared <Effect>(_perPixelEffect));
	perPixelEffectCullNone->setCullMode(device, D3D11_CULL_NONE);

	//Set up Kart Materials and textures
	//Gloss Black
	shared_ptr <Material> glossBlack(new Material(device, XMFLOAT4(0.0, 0.0, 0.0, 1.0)));
	glossBlack->setSpecular(XMFLOAT4(1.0, 1.0, 1.0, 0.1));
	glossBlack->setUsage(USE_MATERIAL_ALL);
	//Helmet - Black Reflective
	shared_ptr <Material> glossBlackReflection(new Material(device, XMFLOAT4(0.0, 0.0, 0.0, 1.0)));
	glossBlackReflection->setSpecular(XMFLOAT4(1.0, 1.0, 1.0, 0.1));
	glossBlackReflection->setUsage(REFLECTION_MAP);

	//Matt Black
	shared_ptr <Material>  mattBlack(new Material(device, XMFLOAT4(0.0, 0.0, 0.0, 1.0)));
	mattBlack->setSpecular(XMFLOAT4(0.2, 0.2, 0.2, 0.001));
	//Matt Grey
	shared_ptr <Material> mattGrey(new Material(device, XMFLOAT4(0.2, 0.3, 0.4, 1.0)));
	mattGrey->setSpecular(XMFLOAT4(0.2, 0.2, 0.2, 0.001));
	//Chrome
	shared_ptr <Material> matChrome(new Material(device, XMFLOAT4(0.0, 0.0, 0.0, 1.0)));
	matChrome->setSpecular(XMFLOAT4(5.0, 5.0, 5.0, 0.005));
	matChrome->setUsage(CHROME);
	//Tire Material
	shared_ptr <Material>  matTire(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
	matTire->setSpecular(XMFLOAT4(0.5, 0.5, 0.5, 0.001));
	matTire->setUsage(DIFFUSE_MAP);
	// Load Tire Texture
	Texture tireTexture = Texture(context, device, ResourcePath + L"Resources\\Models\\Kart\\GoodYearEagle.png");
	matTire->setTexture(tireTexture.getShaderResourceView());

	//Kart Faring Material
	shared_ptr <Material>  matFaring(new Material(device, XMFLOAT4(1.0, 1.0, 1.0, 1.0)));
	matFaring->setSpecular(XMFLOAT4(2.0, 2.0, 2.0, 0.05));
	matFaring->setUsage(DIFFUSE_MAP | REFLECTION_MAP);
	//Setup Kart Faring Texture
	ID3D11ShaderResourceView* faringTextureArray[] = { kartTexture->getShaderResourceView() };
	matFaring->setTextures(faringTextureArray, 1);

	//Load Kart components separately so they can have different materials and PhysX properties
	XMMATRIX FramePreTransMat = XMMatrixScaling(0.001 * scaleKart, 0.001 * scaleKart, 0.001 * scaleKart);
	Frame = new  Model(context,device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPoly.obj"), perPixelEffectCullNone, glossBlack, 0,&FramePreTransMat);
	//Load Faring
	XMMATRIX FaringPreTransMat = XMMatrixScaling(0.0032* scaleKart, 0.0032 * scaleKart, 0.0032 * scaleKart) * XMMatrixRotationX(XMConvertToRadians(-90)) * XMMatrixTranslation(0.0, 0.05, 0.2);
	Farings = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\FaringTexture2.obj"), _perPixelEffect, matFaring, DEFAULT,&FaringPreTransMat);
	
		//Farings = new  Model(context, device, wstring(L"..\\Resources\\Models\\VWBeetle\\Beetle3.obj"), _reflectionEffect, matFaring, DEFAULT, &FaringPreTransMat);

	//Load Engine
	Engine = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPoly.obj"), _perPixelEffect, matChrome,  4,&FramePreTransMat);
	
	//Wheels and Tires
	FLWheel2KartMat = XMMatrixTranslation(-0.7 * scaleKart, 0.18 * scaleKart, 0.75 * scaleKart);
	FRWheel2KartMat = XMMatrixTranslation(0.7 * scaleKart, 0.18 * scaleKart, 0.75 * scaleKart);
	RearTires2KartMat = XMMatrixTranslation(0.0 * scaleKart, 0.18 * scaleKart, -0.65 * scaleKart);
	XMMATRIX Wheel2kartPreTransMat = XMMatrixScaling(0.001 * scaleKart, 0.001 * scaleKart, 0.001 * scaleKart);
	FLWheel = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, glossBlackReflection, 8, &Wheel2kartPreTransMat);
	FRWheel = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, glossBlackReflection, 10, &Wheel2kartPreTransMat);
	FLTire = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, matTire, 9, &Wheel2kartPreTransMat);
	FRTire = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, matTire, 7, &Wheel2kartPreTransMat);
	RearWheels = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, glossBlack, 6, &FramePreTransMat);
	RearTires = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, matTire, 5, &Wheel2kartPreTransMat);
	
	//Load Steering Wheel
	XMMATRIX SWheelPreTransMat = XMMatrixScaling(0.001 * scaleKart, 0.001 * scaleKart, 0.001 * scaleKart) * XMMatrixTranslation(0, 0, -0.65) * XMMatrixRotationX(XMConvertToRadians(35));
	SWheel2KartMat = XMMatrixRotationX(XMConvertToRadians(-35)) * XMMatrixTranslation(0, 0, 0.65);
	SWheel = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, mattBlack, 2,&SWheelPreTransMat);
	SWheelHub = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Kart\\KartLowPolyWC.obj"), _perPixelEffect, mattBlack, 3,&SWheelPreTransMat);
	
	//Load Driver Parts
	XMMATRIX DriverPreTransMat = XMMatrixScaling(0.049 * scaleKart, 0.049 * scaleKart, 0.049 * scaleKart) * XMMatrixRotationX(XMConvertToRadians(-82)) * XMMatrixTranslation(-0.45 * scaleKart, 0.15 * scaleKart, 0.2 * scaleKart);
	XMMATRIX HandsPreTransMat = DriverPreTransMat * XMMatrixTranslation(0, 0, -0.65) * XMMatrixRotationX(XMConvertToRadians(35));
	Hands2KartMat = XMMatrixRotationX(XMConvertToRadians(-35)) * XMMatrixTranslation(0, 0, 0.65);
	Driver = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Driver\\GTR_Driver3Smooth.obj"), _perPixelEffect, mattGrey, 5,&DriverPreTransMat);
	Head = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Driver\\GTR_Driver3Smooth.obj"), _perPixelEffect, glossBlackReflection,  7, &DriverPreTransMat);
	Shoes = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Driver\\GTR_Driver3SmoothFeet.obj"), _perPixelEffect, mattBlack, DEFAULT, &DriverPreTransMat);
	Hands = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Driver\\GTR_Driver3SmoothHands.obj"), _perPixelEffect, mattGrey, DEFAULT, &HandsPreTransMat);
	Legs = new  Model(context, device, wstring(ResourcePath + L"Resources\\Models\\Driver\\GTR_Driver3SmoothLegs.obj"), _perPixelEffect, mattGrey,  DEFAULT, &DriverPreTransMat);

}

void Kart::updateTires(physx::PxTransform wheelTrans[],float stepSize)
{
	physx::PxTransform trans = mVehicle4W->getRigidDynamicActor()->getGlobalPose();
	PxVehicleTireData tires;
	int count = 0;

	for (PxU32 i = 0; i < 4; i++)
	{
		if (ground->getMapColour(trans.p.x + wheelTrans[i].p.x, trans.p.z + wheelTrans[i].p.z).x > 0.0)			//onGrass = true;
		{
			tires.mType = grassTireFriction;
			count++;
		}
		else//onGrass = false;
			tires.mType = trackTireFriction;
		mVehicle4W->mWheelsSimData.setTireData(i, tires);
	}
	//Check if Kart is driving on grass. If all wheels are on grass more 20 time units, disqualify
	if (count >= 4)
	{
		if (timer > 2)
			disc = true;
		else
			timer+=stepSize;
	}
	else
		timer = 0;

	// If at lease 2 wheels are on grass then kart is on grass
	if (count >= 2)
		onGrass = true;
	else
		onGrass = false;
}
void Kart::update(ID3D11DeviceContext* context, float stepSize)
{
	physx::PxTransform trans = mVehicle4W->getRigidDynamicActor()->getGlobalPose();
	trans.p.y += 0.05;

	physx::PxVec3 PXvel = mVehicle4W->getRigidDynamicActor()->getLinearVelocity();
	physx::PxU16 numShapes = mVehicle4W->getRigidDynamicActor()->getNbShapes();
	physx::PxShape* carShapes[5];
	mVehicle4W->getRigidDynamicActor()->getShapes(carShapes, numShapes);
	mVehicle4W->mDriveDynData.mControlAnalogVals;
	physx::PxReal rpm = mVehicle4W->mDriveDynData.getEngineRotationSpeed();
	physx::PxTransform wheelTrans[4] = { carShapes[0]->getLocalPose(),carShapes[1]->getLocalPose(),carShapes[2]->getLocalPose(),carShapes[3]->getLocalPose() };


	updateTires(wheelTrans,stepSize);

	XMVECTOR quart = DirectX::XMLoadFloat4(&XMFLOAT4(trans.q.x, trans.q.y, trans.q.z, trans.q.w));
	//Wheels 0=FL, 1=FR, 2&3=Rear L&R
	XMVECTOR wheelQuart[4] = { DirectX::XMLoadFloat4(&XMFLOAT4(wheelTrans[0].q.x, wheelTrans[0].q.y, wheelTrans[0].q.z, wheelTrans[0].q.w)),
	DirectX::XMLoadFloat4(&XMFLOAT4(wheelTrans[1].q.x, wheelTrans[1].q.y, wheelTrans[1].q.z, wheelTrans[1].q.w)),
	DirectX::XMLoadFloat4(&XMFLOAT4(wheelTrans[2].q.x, wheelTrans[2].q.y, wheelTrans[2].q.z, wheelTrans[2].q.w)),
	DirectX::XMLoadFloat4(&XMFLOAT4(wheelTrans[3].q.x, wheelTrans[3].q.y, wheelTrans[3].q.z, wheelTrans[3].q.w)) };

	float angle = (mWheelQueryResults[0].steerAngle + mWheelQueryResults[1].steerAngle / 2.0);

	//Update smoke
	float smokeScale = 0;
	wheelSpin = false;

	float longSlip = fabs(mWheelQueryResults[2].longitudinalSlip) + fabs(mWheelQueryResults[3].longitudinalSlip);
	if (longSlip > 1.0)
		smokeScale = min(longSlip / 2, 1.0);

	if (!mVehicle4W->getRigidDynamicActor()->isSleeping())
	{
		float latSlip = min(fabs(mWheelQueryResults[2].lateralSlip) + fabs(mWheelQueryResults[3].lateralSlip) * 2, 1.0f);
		if (latSlip > 0.0)
			smokeScale = max(smokeScale, latSlip);
	}
	smokeScale = smokeScale * 0.2 + smokeScaleOld * 0.8;
	smokeScaleOld = smokeScale;
	if (smokeScale > 0.1)
	{
		LSmokeMat = XMMatrixScaling(0.3 * 2 * scaleKart * smokeScale, 0.3 * 2 * scaleKart * smokeScale, 0.3 * 2 * scaleKart * smokeScale) * XMMatrixTranslation(-0.35 * 2 * scaleKart, 0.1 * 2 * scaleKart, -0.4 * 2 * scaleKart) * XMMatrixRotationQuaternion(quart) * XMMatrixTranslation(trans.p.x, trans.p.y, trans.p.z);
		RSmokeMat = XMMatrixScaling(0.3 * 2 * scaleKart * smokeScale, 0.3 * 2 * scaleKart * smokeScale, 0.3 * 2 * scaleKart * smokeScale) * XMMatrixTranslation(0.35 * 2 * scaleKart, 0.1 * 2 * scaleKart, -0.4 * 2 * scaleKart) * XMMatrixRotationQuaternion(quart) * XMMatrixTranslation(trans.p.x, trans.p.y, trans.p.z);
		wheelSpin = true;
		if (onGrass)
			smokeScale *= 1000;
	}


	//Update kart
	XMMATRIX KartGlobalPose = XMMatrixRotationQuaternion(quart) * XMMatrixTranslation(trans.p.x * 1.0, trans.p.y * 1.0, trans.p.z * 1.0);
	
	Frame->setWorldMatrix(KartGlobalPose);
	Frame->update(context);

	Farings->setWorldMatrix( KartGlobalPose);
	Farings->update(context);

	Engine->setWorldMatrix(KartGlobalPose);
	Engine->update(context);

	SWheel->setWorldMatrix( XMMatrixRotationY(angle * 0.5)*SWheel2KartMat* KartGlobalPose);
	SWheel->update(context);
	SWheelHub->setWorldMatrix(XMMatrixRotationY(angle * 0.5) * SWheel2KartMat * KartGlobalPose);
	SWheelHub->update(context);

	FLWheel->setWorldMatrix(XMMatrixRotationQuaternion(wheelQuart[0]) * FLWheel2KartMat*KartGlobalPose);
	FLWheel->update(context);
	FLTire->setWorldMatrix(XMMatrixRotationQuaternion(wheelQuart[0]) * FLWheel2KartMat*KartGlobalPose);
	FLTire->update(context);
	FRWheel->setWorldMatrix(XMMatrixRotationQuaternion(wheelQuart[1]) * FRWheel2KartMat* KartGlobalPose);
	FRWheel->update(context);
	FRTire->setWorldMatrix(XMMatrixRotationQuaternion(wheelQuart[1]) * FRWheel2KartMat* KartGlobalPose);
	FRTire->update(context);	
	RearWheels->setWorldMatrix(KartGlobalPose);
	RearWheels->update(context);
	RearTires->setWorldMatrix( XMMatrixRotationQuaternion(wheelQuart[2]) * RearTires2KartMat * KartGlobalPose);
	RearTires->update(context);

	Driver->setWorldMatrix( XMMatrixRotationZ(angle * -0.05) * KartGlobalPose);
	Driver->update(context);
	Head->setWorldMatrix( XMMatrixRotationZ(angle * -0.08) * KartGlobalPose);
	Head->update(context);
	Shoes->setWorldMatrix( KartGlobalPose);
	Shoes->update(context);
	Hands->setWorldMatrix( XMMatrixRotationY(angle * 0.25) * Hands2KartMat * KartGlobalPose);
	Hands->update(context);
	Legs->setWorldMatrix( KartGlobalPose);
	Legs->update(context);
}
void Kart::updateLapTimes(float stepSize,float startDist,float finishDist, XMVECTOR startFinishPos)
{
	physx::PxTransform trans = mVehicle4W->getRigidDynamicActor()->getGlobalPose();
	XMVECTOR kartPos = XMVectorSet(trans.p.x, trans.p.y, trans.p.z, 1.0);
	if (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(kartPos, startFinishPos))) < finishDist)
	{
		if (getLapTime() > 10)
		{
			if (getLapTime() <getBestLapTime())
				setBestLapTime(getLapTime());
			setLastLapTime(getLapTime());
		}
		setLapTime(0.0f);
		setDisc(false);
	}

	if (DirectX::XMVectorGetX(DirectX::XMVector3Length(DirectX::XMVectorSubtract(kartPos, startFinishPos))) > startDist)
		setLapTime(getLapTime() + stepSize);
	if (getDisc())
		setLapTime(0);
}

void Kart::render(ID3D11DeviceContext* context) 
{
	// Render Driver
	if (Driver)
		Driver->render(context);
	if (Head)
		Head->render(context);
	if (Shoes)
		Shoes->render(context);
	if (Legs)
		Legs->render(context);
	if (Hands)
		Hands->render(context);

	// Render Frame
	if (Frame)
		Frame->render(context);
	if (Farings)
		Farings->render(context);
	if (Engine)
		Engine->render(context);
	if (FLWheel)
		FLWheel->render(context);
	if (FRWheel)
		FRWheel->render(context);
	if (FLTire)
		FLTire->render(context);
	if (FRTire)
		FRTire->render(context);
	if (RearWheels)
		RearWheels->render(context);
	if (RearTires)
		RearTires->render(context);
	if (SWheelHub)
		SWheelHub->render(context);
	if (SWheel)
		SWheel->render(context);
}

Kart::~Kart() 
{
	//Lots of cleanup to do here DirectX, Fmod, PhysX
	if (Frame)
		delete(Frame);
	if (Driver)
		delete(Driver);
	if (Hands)
		delete(Hands);
	if (Head)
		delete(Head);
	if (Shoes)
		delete(Shoes);
	if (Legs)
		delete(Legs);
	if (FLWheel)
		delete(FLWheel);
	if (FRWheel)
		delete(FRWheel);
	if (FLTire)
		delete(FLTire);
	if (FRTire)
		delete(FRTire);
	if (RearWheels)
		delete(RearWheels);
	if (RearTires)
		delete(RearTires);
	if (Engine)
		delete(Engine);
	if (Farings)
		delete(Farings);
	if (SWheel)
		delete(SWheel);
	if (SWheelHub)
		delete(SWheelHub);

	//FMOD::System* FMSystem; Cleaned up in Scene
	channel2->stop();
	channel3->stop();
	engine2->release();
	engine2 = nullptr;
	engine3->release();
	engine3 = nullptr;


}
