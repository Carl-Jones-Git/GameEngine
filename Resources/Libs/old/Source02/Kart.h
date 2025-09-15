
#pragma once

#include <d3d11_2.h>
#include <Effect.h>
#include <Material.h>
#include "Model.h"
#include <Effect.h>
#include <Texture.h>
#include <Terrain.h>
#include <Xinput.h>
#include <FmodStudio\inc\fmod.hpp>
//#include "../PhysXVehicleController.h"
#include <PhysXVehicleController.h>



// Abstract base class to model mesh objects for rendering in DirectX
class Kart {

	//Public Attributes to be made protected with accessor methods
protected:
	//General Game/Graphics Attributes

	Terrain* ground;

	bool started = false;
	bool disc = false;
	float lapTime = 0.0f;
	float bestLapTime = 1000.0f;
	float lastLapTime = 0.0f;
	bool onGrass = false;
	bool wheelSpin = false;
	float smokeScaleOld = 0;
	DirectX::XMMATRIX LSmokeMat;
	DirectX::XMMATRIX RSmokeMat;
	int gridPosition = 0;
	float timer = 0;
	float scaleKart = 0.75;
	XMMATRIX FLWheel2KartMat;
	XMMATRIX FRWheel2KartMat;
	XMMATRIX RearTires2KartMat;
	XMMATRIX SWheel2KartMat;
	XMMATRIX Hands2KartMat;
	//Kart Model components
	Model* Frame = nullptr;
	Model* Driver = nullptr;
	Model* Hands = nullptr;
	Model* Head = nullptr;
	Model* Shoes = nullptr;
	Model* Legs = nullptr;
	Model* FLWheel = nullptr;
	Model* FRWheel = nullptr;
	Model* FLTire = nullptr;
	Model* FRTire = nullptr;
	Model* RearWheels = nullptr;
	Model* RearTires = nullptr;
	Model* Engine = nullptr;
	Model* Farings = nullptr;
	Model* SWheel = nullptr;
	Model* SWheelHub = nullptr;

	//FMod Sound Attributes
	FMOD::System* FMSystem;
	FMOD::Channel* channel2;
	FMOD::Channel* channel3;
	FMOD::Sound* engine2;
	FMOD::Sound* engine3;

	//PhysX attributes
	PxVehicleDrive4WRawInputData mVehicleInputData;
	PxWheelQueryResult mWheelQueryResults[PX_MAX_NB_WHEELS];
	bool mMimicKeyInputs;
	bool mIsVehicleInAir;
	physx::PxVec3 polePosition;
	PxVehicleDrive4W* mVehicle4W;
	int kartCC = 0;
	int trackTireFriction = TIRE_TYPE_NORMAL;
	int grassTireFriction = TIRE_TYPE_WORN;

public:
	//Public methods
	Kart(FMOD::System* _FMSystem, ID3D11Device* device, ID3D11DeviceContext* context, Terrain* ground, shared_ptr<Effect>  _perPixelEffect,  shared_ptr<Texture>_texture);
	~Kart();
	void render(ID3D11DeviceContext* context);
	void update(ID3D11DeviceContext* context, float stepSize);
	void updateTires(physx::PxTransform wheelTrans[], float stepSize);
	void restart(physx::PxVec3 p = physx::PxVec3(36, 1, 0.7f), physx::PxQuat q = physx::PxQuat(XMConvertToRadians(-100), PxVec3(0, 1, 0)))
	{
		//physx::PxQuat Q(XMConvertToRadians(-100), PxVec3(0, 1, 0));
		physx::PxTransform trans(p, q);
		mVehicle4W->getRigidDynamicActor()->setGlobalPose(trans);
		mVehicle4W->getRigidDynamicActor()->setLinearVelocity(physx::PxVec3(0, 0, 0));
		mVehicle4W->getRigidDynamicActor()->setAngularVelocity(physx::PxVec3(0, 0, 0));
		mVehicle4W->setToRestState();
	}
	void setStartPosition(physx::PxVec3 _polePosition, int _gridPosition = 0) {
		polePosition = _polePosition;
		gridPosition = _gridPosition;
	};

	//Getters
	Model* getFLTire(){return FLTire;};
	Model* getFRTire() { return FRTire; };
	Model* getRearTires() { return RearTires; };
	Model* getFarings() { return Farings; };
	Model* getFrame() { return Frame; };
	Model* getDriver() { return Driver; };
	Model* getHead() { return Head; };
	bool getOnGrass() { return onGrass; };
	bool getStarted() { return started; };
	bool getDisc() { return disc; };
	float getLapTime() { return lapTime; };
	float getBestLapTime() { return bestLapTime; };
	float getLastLapTime() { return lastLapTime; };
	DirectX::XMMATRIX getLSmokeMat()  { return LSmokeMat; };
	DirectX::XMMATRIX getRSmokeMat() { return RSmokeMat; };
	bool getWheelSpin() { return wheelSpin; };
	int getKartCC() { return kartCC; };
	PxVehicleDrive4WRawInputData* getVehicleInputData() { return &mVehicleInputData; }
	bool getMimicKeyInputs() { return mMimicKeyInputs; };
	physx::PxVec3 getPolePosition() { return polePosition; };
	PxVehicleDrive4W* getVehicle4W() { return mVehicle4W; };
	PxWheelQueryResult* getWheelQueryResults() { return mWheelQueryResults; };
	bool getIsVehicleInAir() { return mIsVehicleInAir; };
	void updateLapTimes(float stepSize, float startDist, float finishDist, XMVECTOR startFinishPos);

	//Setters
	void setKartCC(int _kartCC) { kartCC=_kartCC; };
	void setStarted(bool _started) { started = _started; };
	void setDisc(bool _disc) { disc = _disc; };
	void setLapTime(float _lapTime) {lapTime =_lapTime; };
	void setBestLapTime(float _bestLapTime) { bestLapTime= _bestLapTime; };
	void setLastLapTime(float _lastLapTime) { lastLapTime=_lastLapTime; };
	void setIsVehicleInAir(bool _IsVehicleInAir) {mIsVehicleInAir = _IsVehicleInAir;};
	void setVehicle4W(PxVehicleDrive4W* _Vehicle4W) { mVehicle4W = _Vehicle4W; };
};
