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
#include "PhysXVehicleController.h"

extern PxPhysics* mPhysics;
extern PxScene* mScene;
extern PxCooking* gCooking;
extern PxMaterial* mMaterial;
extern PxBatchQuery* gBatchQuery;
extern VehicleSceneQueryData* gVehicleSceneQueryData;


//Tire model friction for each combination of drivable surface type and tire type.defaultMaterial
extern PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs;

ActorUserData gActorUserData[NUM_VEHICLES];
ShapeUserData gShapeUserDatas[NUM_VEHICLES][PX_MAX_NB_WHEELS];

PxVehicleDrive4W* PhysXVehicleController::initVehiclePX(float x, float y, float z, float r,int id)
{
    PxVehicleDrive4W* Vehicle4W;
    //Create a vehicle that will drive on the plane.
    physx::PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS|COLLISION_FLAG_OBSTACLE, 0, 0);
    physx::PxFilterData wheelSimFilterData(COLLISION_FLAG_WHEEL, COLLISION_FLAG_GROUND, physx::PxPairFlag::eMODIFY_CONTACTS, 0);
    VehicleDesc vehicleDesc = initVehicleDesc(mMaterial, chassisSimFilterData, wheelSimFilterData, id);
    Vehicle4W = createVehicle4W(vehicleDesc, mPhysics, gCooking);
    PxQuat Q(XMConvertToRadians(r), PxVec3(0, 1, 0));
    physx::PxTransform startTransform(physx::PxVec3(x, (vehicleDesc.chassisDims.y * 0.5f + vehicleDesc.wheelRadius + 1.0f)+y, z), Q);
    Vehicle4W->getRigidDynamicActor()->setGlobalPose(startTransform);
    mScene->addActor(*Vehicle4W->getRigidDynamicActor());
    //Set the vehicle to use auto-gears.
    Vehicle4W->setToRestState();
    Vehicle4W->mDriveDynData.setUseAutoGears(true);
    //Return the reference to the car that was created
    return  Vehicle4W;
}


int setCC(Kart *kart,int kartCC)
{
        physx::PxVehicleEngineData   engine = kart->getVehicle4W()->mDriveSimData.getEngineData();
        physx::PxVehicleGearsData   gears = kart->getVehicle4W()->mDriveSimData.getGearsData();
        if (kartCC == 0)//150cc
        {
            engine.mPeakTorque = 11.0f * 4.5;
            engine.mMaxOmega = 300.0f;//600 approx 6000 rpm
            gears.mNbRatios = 3;
            gears.mFinalRatio = 0.5f;

        }
        else if (kartCC == 1)//200cc
        {
            engine.mPeakTorque = 16.0f * 4.5f;
            engine.mMaxOmega = 300.0f;//approx 6000 rpm
            gears.mNbRatios = 3;
        }
        else if (kartCC == 2)//270cc
        {
            engine.mPeakTorque = 20.0f * 4.5f;
            engine.mMaxOmega = 350.0f;//approx 6000 rpm
            gears.mNbRatios = 3;
        }
        kart->getVehicle4W()->mDriveSimData.setEngineData(engine);
        kart->getVehicle4W()->mDriveSimData.setGearsData(gears);
        return kartCC;
}

PxF32 gSteerVsForwardSpeedData[2 * 8] =
{
    0.0f,		0.5f,
    5.0f,		0.5f,
    10.0f,		0.45f,//0.3f,
    30.0f,		0.4,//0.21f,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32
};

PxFixedSizeLookupTable<8> gSteerVsForwardSpeedTable(gSteerVsForwardSpeedData, 4);

PxVehicleKeySmoothingData gKeySmoothingData =
{
    {
        6.0f,	//rise rate eANALOG_INPUT_ACCEL
        6.0f,	//rise rate eANALOG_INPUT_BRAKE		
        6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE	
        0.6f,	//rise rate eANALOG_INPUT_STEER_LEFT
        0.6f,	//rise rate eANALOG_INPUT_STEER_RIGHT
    },
    {
        10.0f,	//fall rate eANALOG_INPUT_ACCEL
        10.0f,	//fall rate eANALOG_INPUT_BRAKE		
        10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE	
        5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
        5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
    }
};

PxVehiclePadSmoothingData gPadSmoothingData =
{
    {
        6.0f,	//rise rate eANALOG_INPUT_ACCEL
        6.0f,	//rise rate eANALOG_INPUT_BRAKE		
        6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE	
        1.0f,	//rise rate eANALOG_INPUT_STEER_LEFT
        1.0f,	//rise rate eANALOG_INPUT_STEER_RIGHT
    },
    {
        10.0f,	//fall rate eANALOG_INPUT_ACCEL
        10.0f,	//fall rate eANALOG_INPUT_BRAKE		
        10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE	
        5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
        5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
    }
};

void PhysXVehicleController::stepVehicles(float timestep, Kart* kart[], const int numKarts)
{

    //Update the control inputs for the vehicle.
    for (int i = 0; i < numKarts; i++)
    {
        if (kart[i]->getMimicKeyInputs())
            PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(gKeySmoothingData, gSteerVsForwardSpeedTable,*kart[i]->getVehicleInputData(), timestep, kart[i]->getIsVehicleInAir(), *(kart[i]->getVehicle4W()));
        else
            PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(gPadSmoothingData, gSteerVsForwardSpeedTable, *kart[i]->getVehicleInputData(), timestep, kart[i]->getIsVehicleInAir(), *(kart[i]->getVehicle4W()));
    }
    //Update all vehicle wheels in a batch 
    //Raycasts to check for wheel collisions
    PxVehicleWheels* vehicles[NUM_VEHICLES];
    for (int i = 0; i < numKarts; i++)
        vehicles[i] = kart[i]->getVehicle4W();

    PxRaycastQueryResult* raycastResults = gVehicleSceneQueryData->getRaycastQueryResultBuffer(0);
    const PxU32 raycastResultsSize = gVehicleSceneQueryData->getQueryResultBufferSize();
    PxVehicleSuspensionRaycasts(gBatchQuery, numKarts, vehicles, raycastResultsSize, raycastResults);

    //Vehicle update.
    const PxVec3 grav = mScene->getGravity();
    PxVehicleWheelQueryResult vehicleQueryResults[NUM_VEHICLES];
    for (int i = 0; i < numKarts; i++)
        vehicleQueryResults[i] = { kart[i]->getWheelQueryResults(), kart[i]->getVehicle4W()->mWheelsSimData.getNbWheels() };

    PxVehicleUpdates(timestep, grav, *gFrictionPairs, numKarts, vehicles, vehicleQueryResults);
    //Work out if the vehicle is in the air.
    for (int i = 0; i < numKarts; i++)
        kart[i]->setIsVehicleInAir( kart[i]->getVehicle4W()->getRigidDynamicActor()->isSleeping() ? false : PxVehicleIsInAir(vehicleQueryResults[i]));
}


VehicleDesc PhysXVehicleController::initVehicleDesc(PxMaterial* material,const PxFilterData& chassisSimFilterData, const PxFilterData& wheelSimFilterData, const PxU32 vehicleId)
{
    //Set up the chassis mass, dimensions, moment of inertia, and center of mass offset.
    //The moment of inertia is just the moment of inertia of a cuboid but modified for easier steering.
    //Center of mass offset is 0.65m above the base of the chassis and 0.25m towards the front.
 
    const PxF32 chassisMass = 250.0f;
    const PxVec3 chassisDims(1.2f, 0.2f, 1.8f);
    const PxVec3 chassisMOI
    ((chassisDims.y * chassisDims.y + chassisDims.z * chassisDims.z) * chassisMass / 12.0f,
        (chassisDims.x * chassisDims.x + chassisDims.z * chassisDims.z) * 0.8f * chassisMass / 12.0f,
        (chassisDims.x * chassisDims.x + chassisDims.y * chassisDims.y) * chassisMass / 12.0f);
    const PxVec3 chassisCMOffset(0.0f, -chassisDims.y * 0.5f , 0.25f);

    //Set up the wheel mass, radius, width, moment of inertia, and number of wheels.
    //Moment of inertia is just the moment of inertia of a cylinder.
    const PxF32 wheelMass = 2.50f;
    const PxF32 wheelRadius = 0.18f;
    const PxF32 wheelWidth = 0.2f;
   const PxF32 wheelMOI = 0.5f * wheelMass * wheelRadius * wheelRadius;
    const PxU32 nbWheels = 4;
    VehicleDesc vehicleDesc;

    vehicleDesc.chassisMass = chassisMass;
    vehicleDesc.chassisDims = chassisDims;
    vehicleDesc.chassisMOI = chassisMOI;
    vehicleDesc.chassisCMOffset = chassisCMOffset;
    vehicleDesc.chassisMaterial = material;
    vehicleDesc.chassisSimFilterData = chassisSimFilterData;

    vehicleDesc.wheelMass = wheelMass;
    vehicleDesc.wheelRadius = wheelRadius;
    vehicleDesc.wheelWidth = wheelWidth;
    vehicleDesc.wheelMOI = wheelMOI;
    vehicleDesc.numWheels = nbWheels;
    vehicleDesc.wheelMaterial = material;
    vehicleDesc.wheelSimFilterData = wheelSimFilterData;
    vehicleDesc.actorUserData = &gActorUserData[vehicleId];
    vehicleDesc.shapeUserDatas = gShapeUserDatas[vehicleId];

    return vehicleDesc;
}



PxConvexMesh* PhysXVehicleController::createChassisMesh(const PxVec3 dims, PxPhysics& physics, PxCooking& cooking)
{
    float zOffset = 0.15f;
    const PxF32 x = dims.x * 0.5f;
    const PxF32 y = dims.y * 0.5f;
    const PxF32 z = dims.z * 0.5f;
    PxVec3 verts[8] =
    {
        PxVec3(x,y,-z + zOffset),
        PxVec3(x,y,z + zOffset),
        PxVec3(x,-y,z + zOffset),
        PxVec3(x,-y,-z + zOffset),
        PxVec3(-x,y,-z + zOffset),
        PxVec3(-x,y,z + zOffset),
        PxVec3(-x,-y,z + zOffset),
        PxVec3(-x,-y,-z + zOffset)
    };
    return createConvexMesh(verts, 8, physics, cooking);
}
PxConvexMesh* PhysXVehicleController::createWheelMesh(const PxF32 width, const PxF32 radius, PxPhysics& physics, PxCooking& cooking)
{
    PxVec3 points[2 * 16];
    for (PxU32 i = 0; i < 16; i++)
    {
        const PxF32 cosTheta = PxCos(i * PxPi * 2.0f / 16.0f);
        const PxF32 sinTheta = PxSin(i * PxPi * 2.0f / 16.0f);
        const PxF32 y = radius * cosTheta;
        const PxF32 z = radius * sinTheta;
        points[2 * i + 0] = PxVec3(-width / 2.0f, y, z);
        points[2 * i + 1] = PxVec3(+width / 2.0f, y, z);
    }

    return createConvexMesh(points, 32, physics, cooking);
}


void PhysXVehicleController::computeWheelCenterActorOffsets4W(const PxF32 wheelFrontZ, const PxF32 wheelRearZ, const PxVec3& chassisDims, const PxF32 wheelWidth, const PxF32 wheelRadius, const PxU32 numWheels, PxVec3* wheelCentreOffsets)
{
    //chassisDims.z is the distance from the rear of the chassis to the front of the chassis.
    //The front has z = 0.5*chassisDims.z and the rear has z = -0.5*chassisDims.z.
    //Compute a position for the front wheel and the rear wheel along the z-axis.
    //Compute the separation between each wheel along the z-axis.
    const PxF32 numLeftWheels = numWheels / 2.0f;
    const PxF32 deltaZ = (wheelFrontZ - wheelRearZ) / (numLeftWheels - 1.0f);
    //Set the outside of the left and right wheels to be flush with the chassis.
    //Set the top of the wheel to be just touching the underside of the chassis.
    //Begin by setting the rear-left/rear-right/front-left,front-right wheels.
    wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_LEFT] = PxVec3((-chassisDims.x - wheelWidth) * 0.5f-0.1, -(chassisDims.y / 2 + wheelRadius/2)+0.2, wheelRearZ + 0 * deltaZ * 0.5f);
    wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eREAR_RIGHT] = PxVec3((+chassisDims.x + wheelWidth) * 0.5f + 0.1, -(chassisDims.y / 2 + wheelRadius/2) + 0.2, wheelRearZ + 0 * deltaZ * 0.5f);
    wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_LEFT] = PxVec3((-chassisDims.x - wheelWidth) * 0.5f - 0.1, -(chassisDims.y / 2 + wheelRadius/2) + 0.2, wheelRearZ + (numLeftWheels - 1) * deltaZ);
    wheelCentreOffsets[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT] = PxVec3((+chassisDims.x + wheelWidth) * 0.5f + 0.1, -(chassisDims.y / 2 + wheelRadius/2) + 0.2, wheelRearZ + (numLeftWheels - 1) * deltaZ);
    
    //Set the remaining wheels.
    for (PxU32 i = 2, wheelCount = 4; i < numWheels - 2; i += 2, wheelCount += 2)
    {
        wheelCentreOffsets[wheelCount + 0] = PxVec3((-chassisDims.x + wheelWidth) * 0.5f, -(chassisDims.y / 2 + wheelRadius), wheelRearZ + i * deltaZ * 0.5f);
        wheelCentreOffsets[wheelCount + 1] = PxVec3((+chassisDims.x - wheelWidth) * 0.5f, -(chassisDims.y / 2 + wheelRadius), wheelRearZ + i * deltaZ * 0.5f);
    }
}

void PhysXVehicleController::setupWheelsSimulationData(const PxF32 wheelMass, const PxF32 wheelMOI, const PxF32 wheelRadius, const PxF32 wheelWidth,
    const PxU32 numWheels, const PxVec3* wheelCenterActorOffsets,
    const PxVec3& chassisCMOffset, const PxF32 chassisMass,
    PxVehicleWheelsSimData* wheelsSimData)
{
    //Set up the wheels.
    PxVehicleWheelData wheels[PX_MAX_NB_WHEELS];
    {
        //Set up the wheel data structures with mass, moi, radius, width.
        for (PxU32 i = 0; i < numWheels; i++)
        {
           // wheels[i].mDampingRate = 0.0001;
            wheels[i].mMass = wheelMass;// 0.0001;// ;
            wheels[i].mMOI = wheelMOI;
            wheels[i].mRadius = wheelRadius;
            wheels[i].mWidth = wheelWidth;
        }

        //Enable the handbrake for the rear wheels only.
        wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque = 4000.0f;
        wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque = 4000.0f;
        wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxBrakeTorque = 100.0f;
        wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxBrakeTorque = 100.0f;
        //Enable steering for the front wheels only.
        wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer = XMConvertToRadians(75);// PxPi * 0.53;// 0.3333f;
        wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer = XMConvertToRadians(75); //PxPi * 0.53;// 0.3333f;
        wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxBrakeTorque = 0.0f;// 0.3333f;
        wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxBrakeTorque = 0.0f;// 0.3333f;
    }

    //Set up the tires.
    PxVehicleTireData tires[PX_MAX_NB_WHEELS];
    {
        //Set up the tires.
        for (PxU32 i = 0; i < numWheels; i++)
        {
            tires[i].mType = TIRE_TYPE_NORMAL;
        }
    }

    //Set up the suspensions
    PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
    {
        //Compute the mass supported by each suspension spring.
        PxF32 suspSprungMasses[PX_MAX_NB_WHEELS];
        PxVehicleComputeSprungMasses
        (numWheels, wheelCenterActorOffsets,
            chassisCMOffset, chassisMass, 1, suspSprungMasses);

        //Set the suspension data.
        for (PxU32 i = 0; i < numWheels; i++)
        {
            suspensions[i].mMaxCompression = 0.02f;
            suspensions[i].mMaxDroop = 0.02f;
            suspensions[i].mSpringStrength = 3.0f;
            suspensions[i].mSpringDamperRate = 0.0000001f;
            suspensions[i].mSprungMass = suspSprungMasses[i];
        }

        //Set the camber angles.
        const PxF32 camberAngleAtRest = 0.0;
        const PxF32 camberAngleAtMaxDroop = 0.01f;
        const PxF32 camberAngleAtMaxCompression = -0.01f;
        for (PxU32 i = 0; i < numWheels; i += 2)
        {
            suspensions[i + 0].mCamberAtRest = camberAngleAtRest;
            suspensions[i + 1].mCamberAtRest = -camberAngleAtRest;
            suspensions[i + 0].mCamberAtMaxDroop = camberAngleAtMaxDroop;
            suspensions[i + 1].mCamberAtMaxDroop = -camberAngleAtMaxDroop;
            suspensions[i + 0].mCamberAtMaxCompression = camberAngleAtMaxCompression;
            suspensions[i + 1].mCamberAtMaxCompression = -camberAngleAtMaxCompression;
        }
    }

    //Set up the wheel geometry.
    PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
    PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
    PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
    PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
    {
        //Set the geometry data.
        for (PxU32 i = 0; i < numWheels; i++)
        {
            //Vertical suspension travel.
            suspTravelDirections[i] = PxVec3(0, -1, 0);

            //Wheel center offset is offset from rigid body center of mass.
            wheelCentreCMOffsets[i] =
                wheelCenterActorOffsets[i] - chassisCMOffset;

            //Suspension force application point 0.3 metres below 
            //rigid body center of mass.
            suspForceAppCMOffsets[i] =
                PxVec3(wheelCentreCMOffsets[i].x, -0.05f, wheelCentreCMOffsets[i].z);

            //Tire force application point 0.3 metres below 
            //rigid body center of mass.
            tireForceAppCMOffsets[i] =
                PxVec3(wheelCentreCMOffsets[i].x, -0.05f, wheelCentreCMOffsets[i].z);
        }
    }

    //Set up the filter data of the raycast that will be issued by each suspension.
    PxFilterData qryFilterData;
    setupNonDrivableSurface(qryFilterData);

    //Set the wheel, tire and suspension data.
    //Set the geometry data.
    //Set the query filter data
    for (PxU32 i = 0; i < numWheels; i++)
    {
        wheelsSimData->setWheelData(i, wheels[i]);
        wheelsSimData->setTireData(i, tires[i]);
        wheelsSimData->setSuspensionData(i, suspensions[i]);
        wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
        wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
        wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
        wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
        wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
        wheelsSimData->setWheelShapeMapping(i, PxI32(i));
    }

    //Add a front and rear anti-roll bar
    PxVehicleAntiRollBarData barFront;
    barFront.mWheel0 = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
    barFront.mWheel1 = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
    barFront.mStiffness = 1000.0f;
    wheelsSimData->addAntiRollBarData(barFront);
    PxVehicleAntiRollBarData barRear;
    barRear.mWheel0 = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
    barRear.mWheel1 = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
    barRear.mStiffness = 1000.0f;
    wheelsSimData->addAntiRollBarData(barRear);
}



PxRigidDynamic* PhysXVehicleController::createVehicleActor
(const PxVehicleChassisData& chassisData,
    PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData,
    PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData,
    PxPhysics& physics)
{
    //We need a rigid body actor for the vehicle.
    //Don't forget to add the actor to the scene after setting up the associated vehicle.
    PxRigidDynamic* vehActor = physics.createRigidDynamic(PxTransform(PxIdentity));

    //Wheel and chassis query filter data.
    //Optional: cars don't drive on other cars.
    PxFilterData wheelQryFilterData;
    setupNonDrivableSurface(wheelQryFilterData);
    PxFilterData chassisQryFilterData;
    setupNonDrivableSurface(chassisQryFilterData);

    //Add all the wheel shapes to the actor.
    for (PxU32 i = 0; i < numWheels; i++)
    {
        PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
        PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*vehActor, geom, *wheelMaterials[i]);
        wheelShape->setQueryFilterData(wheelQryFilterData);
        wheelShape->setSimulationFilterData(wheelSimFilterData);
        wheelShape->setLocalPose(PxTransform(PxIdentity));
    }

    //Add the chassis shapes to the actor.
    for (PxU32 i = 0; i < numChassisMeshes; i++)
    {
        PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*vehActor, PxConvexMeshGeometry(chassisConvexMeshes[i]), *chassisMaterials[i]);
        chassisShape->setQueryFilterData(chassisQryFilterData);
        chassisShape->setSimulationFilterData(chassisSimFilterData);
        chassisShape->setLocalPose(PxTransform(PxIdentity));
    }

    vehActor->setMass(chassisData.mMass);
    vehActor->setMassSpaceInertiaTensor(chassisData.mMOI);
    vehActor->setCMassLocalPose(PxTransform(chassisData.mCMOffset, PxQuat(PxIdentity)));

    return vehActor;
}

PxVehicleDrive4W* PhysXVehicleController::createVehicle4W(const VehicleDesc& vehicle4WDesc, PxPhysics* physics, PxCooking* cooking)
{
   const PxVec3 chassisDims = vehicle4WDesc.chassisDims;
    const PxF32 wheelWidth = vehicle4WDesc.wheelWidth;
    const PxF32 wheelRadius = vehicle4WDesc.wheelRadius;
    const PxU32 numWheels = vehicle4WDesc.numWheels;

    const PxFilterData& chassisSimFilterData = vehicle4WDesc.chassisSimFilterData;
    const PxFilterData& wheelSimFilterData = vehicle4WDesc.wheelSimFilterData;

    //Construct a physx actor with shapes for the chassis and wheels.
    //Set the rigid body mass, moment of inertia, and center of mass offset.
    PxRigidDynamic* veh4WActor = NULL;
    {
        //Construct a convex mesh for a cylindrical wheel.
        PxConvexMesh* wheelMesh = createWheelMesh(wheelWidth, wheelRadius, *physics, *cooking);
        //Assume all wheels are identical for simplicity.
        PxConvexMesh* wheelConvexMeshes[PX_MAX_NB_WHEELS];
        PxMaterial* wheelMaterials[PX_MAX_NB_WHEELS];

        //Set the meshes and materials for the driven wheels.
        for (PxU32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i <= PxVehicleDrive4WWheelOrder::eREAR_RIGHT; i++)
        {
            wheelConvexMeshes[i] = wheelMesh;
            wheelMaterials[i] = vehicle4WDesc.wheelMaterial;
        }
        //Set the meshes and materials for the non-driven wheels
        for (PxU32 i = PxVehicleDrive4WWheelOrder::eREAR_RIGHT + 1; i < numWheels; i++)
        {
            wheelConvexMeshes[i] = wheelMesh;
            wheelMaterials[i] = vehicle4WDesc.wheelMaterial;
        }

        //Chassis just has a single convex shape for simplicity.
        PxConvexMesh* chassisConvexMesh = createChassisMesh(chassisDims, *physics, *cooking);
        PxConvexMesh* chassisConvexMeshes[1] = { chassisConvexMesh };
        PxMaterial* chassisMaterials[1] = { vehicle4WDesc.chassisMaterial };

        //Rigid body data.
        PxVehicleChassisData rigidBodyData;
        rigidBodyData.mMOI = vehicle4WDesc.chassisMOI;
        rigidBodyData.mMass = vehicle4WDesc.chassisMass;
        rigidBodyData.mCMOffset = vehicle4WDesc.chassisCMOffset;

        veh4WActor = createVehicleActor
        (rigidBodyData,
            wheelMaterials, wheelConvexMeshes, numWheels, wheelSimFilterData,
            chassisMaterials, chassisConvexMeshes, 1, chassisSimFilterData,
            *physics);
    }

    //Set up the sim data for the wheels.
    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(numWheels);
    {
        //Compute the wheel center offsets from the origin.
        PxVec3 wheelCenterActorOffsets[PX_MAX_NB_WHEELS];
        const PxF32 frontZ = chassisDims.z * 0.3f;
        const PxF32 rearZ = -chassisDims.z * 0.3f;
        computeWheelCenterActorOffsets4W(frontZ, rearZ, chassisDims, wheelWidth, wheelRadius, numWheels, wheelCenterActorOffsets);

        //Set up the simulation data for all wheels.
        setupWheelsSimulationData(vehicle4WDesc.wheelMass, vehicle4WDesc.wheelMOI, wheelRadius, wheelWidth,
            numWheels, wheelCenterActorOffsets, vehicle4WDesc.chassisCMOffset, vehicle4WDesc.chassisMass,
            wheelsSimData);
    }

    //Set up the sim data for the vehicle drive model.
    PxVehicleDriveSimData4W driveSimData;
    {
        //Diff
        PxVehicleDifferential4WData diff;
        diff.mType = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
        diff.mRearBias = 1.0f;
        driveSimData.setDiffData(diff);

        //Engine
        PxVehicleEngineData engine;
        engine.mPeakTorque = 11.0*4.5f;
        engine.mMaxOmega = 300.0f;//approx 6000 rpm
        engine.mMOI = 0.5;
        engine.mDampingRateZeroThrottleClutchEngaged = 1.0f;// 0.5;
        engine.mDampingRateZeroThrottleClutchDisengaged = 1.0f;// 0.01;
        cout << engine.mMOI << endl;//1
        cout <<engine.mDampingRateZeroThrottleClutchEngaged << endl;//2
        cout <<engine.mDampingRateZeroThrottleClutchDisengaged << endl;//0.35


        driveSimData.setEngineData(engine);

        //Gears
        PxVehicleGearsData gears;
        gears.mSwitchTime = 0.0f;
        gears.mNbRatios = 3.0f;
        gears.mFinalRatio = 0.5f;
        driveSimData.setGearsData(gears);

        //Clutch
        PxVehicleClutchData clutch;
        clutch.mStrength = 0.8f;
        driveSimData.setClutchData(clutch);
    

        //Ackermann steer accuracy
        PxVehicleAckermannGeometryData ackermann;
        ackermann.mAccuracy = 1.0f;
        ackermann.mAxleSeparation =
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).z -
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).z;
        ackermann.mFrontWidth =
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).x -
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x;
        ackermann.mRearWidth =
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).x -
            wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x;
        driveSimData.setAckermannGeometryData(ackermann);
    }

    //Create a vehicle from the wheels and drive sim data.
    PxVehicleDrive4W* vehDrive4W = PxVehicleDrive4W::allocate(numWheels);
    vehDrive4W->setup(physics, veh4WActor, *wheelsSimData, driveSimData, numWheels - 4);

    //Configure the userdata
    configureUserData(vehDrive4W, vehicle4WDesc.actorUserData, vehicle4WDesc.shapeUserDatas);

    //Free the sim data because we don't need that any more.
    wheelsSimData->free();

    return vehDrive4W;
}

void PhysXVehicleController::configureUserData(PxVehicleWheels* vehicle, ActorUserData* actorUserData, ShapeUserData* shapeUserDatas)
{
    if (actorUserData)
    {
        vehicle->getRigidDynamicActor()->userData = actorUserData;
        actorUserData->vehicle = vehicle;
    }

    if (shapeUserDatas)
    {
        PxShape* shapes[PX_MAX_NB_WHEELS + 1];
        vehicle->getRigidDynamicActor()->getShapes(shapes, PX_MAX_NB_WHEELS + 1);
        for (PxU32 i = 0; i < vehicle->mWheelsSimData.getNbWheels(); i++)
        {
            const PxI32 shapeId = vehicle->mWheelsSimData.getWheelShapeMapping(i);
            shapes[shapeId]->userData = &shapeUserDatas[i];
            shapeUserDatas[i].isWheel = true;
            shapeUserDatas[i].wheelId = i;
        }
    }
}
//
//Controls
void PhysXVehicleController::startAccelerateReverseMode(Kart* kart)
{
    kart->getVehicle4W()->mDriveDynData.forceGearChange(PxVehicleGearsData::eREVERSE);

    if (kart->getMimicKeyInputs())
    {
        kart->getVehicleInputData()->setDigitalAccel(true);
    }
    else
    {
        kart->getVehicleInputData()->setAnalogAccel(1.0f);
    }
}

void PhysXVehicleController::startTurnHardLeftMode(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
         kart->getVehicleInputData()->setDigitalAccel(true);
         kart->getVehicleInputData()->setDigitalSteerLeft(true);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogAccel(true);
         kart->getVehicleInputData()->setAnalogSteer(-1.0f);
    }
}
void PhysXVehicleController::startHandbrakeTurnLeftMode(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
         kart->getVehicleInputData()->setDigitalSteerLeft(true);
         kart->getVehicleInputData()->setDigitalHandbrake(true);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogSteer(-1.0f);
         kart->getVehicleInputData()->setAnalogHandbrake(1.0f);
    }
}
void PhysXVehicleController::startHandbrakeTurnRightMode(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
         kart->getVehicleInputData()->setDigitalSteerRight(true);
         kart->getVehicleInputData()->setDigitalHandbrake(true);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogSteer(1.0f);
         kart->getVehicleInputData()->setAnalogHandbrake(1.0f);
    }
}

void PhysXVehicleController::startAccelerateForwardsMode(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
         kart->getVehicleInputData()->setDigitalAccel(true);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogAccel(1.0f);
    }
}
void PhysXVehicleController::startTurnHardRightMode(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
         kart->getVehicleInputData()->setDigitalAccel(true);
         kart->getVehicleInputData()->setDigitalSteerRight(true);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogAccel(1.0f);
         kart->getVehicleInputData()->setAnalogSteer(1.0f);
    }
}


void PhysXVehicleController::startBrakeMode(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
         kart->getVehicleInputData()->setDigitalBrake(true);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogBrake(1.0f);
    }
}

void PhysXVehicleController::releaseAllControls(Kart* kart)
{
    if (kart->getMimicKeyInputs())
    {
        //gVehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eNEUTRAL);
         kart->getVehicleInputData()->setDigitalAccel(false);
         kart->getVehicleInputData()->setDigitalSteerLeft(false);
         kart->getVehicleInputData()->setDigitalSteerRight(false);
         kart->getVehicleInputData()->setDigitalBrake(false);
         kart->getVehicleInputData()->setDigitalHandbrake(false);
    }
    else
    {
         kart->getVehicleInputData()->setAnalogAccel(0.0f);
         kart->getVehicleInputData()->setAnalogSteer(0.0f);
         kart->getVehicleInputData()->setAnalogBrake(0.0f);
         kart->getVehicleInputData()->setAnalogHandbrake(0.0f);
    }
}




