#pragma once

#include <iostream>
#include "PhysXVehicleUtils.h"
#include "PhysXVehicleController.h"

class PhysXKarting
{
public:
	//PxPhysics* mPhysics = NULL;
	//PxScene* mScene = NULL;
	//PxCooking* mCooking = NULL;
	//PhysXKarting();

	int initScenePX();
	void CreateHeightField(ExtendedVertexStruct* terrain,int sizeWH =50, float scaleXZ = 4.0f, float scaleY = 0.3f);
	void  step(float timestep);
	~PhysXKarting();
};

