/*
 * Original PhysX code:
 * Copyright (c) 2008-2021 NVIDIA Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Modified by Carl Jones (2023) for educational game engine project.
*/

#pragma once
#include <PxPhysicsAPI.h>
#include <PxFiltering.h>
#include "PhysXVehicleUtils.h"
#include "Kart.h"

class Kart;

#define NUM_AI_VEHICLES 6
#define NUM_VEHICLES NUM_AI_VEHICLES+1
using namespace physx;


int setCC(Kart* kart, int cc);


enum DriveMode
{
    eDRIVE_MODE_ACCEL_FORWARDS = 0,
    eDRIVE_MODE_ACCEL_REVERSE,
    eDRIVE_MODE_HARD_TURN_LEFT,
    eDRIVE_MODE_HANDBRAKE_TURN_LEFT,
    eDRIVE_MODE_HARD_TURN_RIGHT,
    eDRIVE_MODE_HANDBRAKE_TURN_RIGHT,
    eDRIVE_MODE_BRAKE,
    eDRIVE_MODE_NONE
};


enum
{
    COLLISION_FLAG_GROUND = 1 << 0,
    COLLISION_FLAG_WHEEL = 1 << 1,
    COLLISION_FLAG_CHASSIS = 1 << 2,
    COLLISION_FLAG_OBSTACLE = 1 << 3,
    COLLISION_FLAG_DRIVABLE_OBSTACLE = 1 << 4,

    COLLISION_FLAG_GROUND_AGAINST = COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
    COLLISION_FLAG_WHEEL_AGAINST = COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE,
    COLLISION_FLAG_CHASSIS_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
    COLLISION_FLAG_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_WHEEL | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE,
    COLLISION_FLAG_DRIVABLE_OBSTACLE_AGAINST = COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE | COLLISION_FLAG_DRIVABLE_OBSTACLE
};

struct VehicleDesc
{
    VehicleDesc()
        : chassisMass(0.0f),
        chassisDims(PxVec3(0.0f, 0.0f, 0.0f)),
        chassisMOI(PxVec3(0.0f, 0.0f, 0.0f)),
        chassisCMOffset(PxVec3(0.0f, 0.0f, 0.0f)),
        chassisMaterial(NULL),
        wheelMass(0.0f),
        wheelWidth(0.0f),
        wheelRadius(0.0f),
        wheelMOI(0.0f),
        wheelMaterial(NULL),
        actorUserData(NULL),
        shapeUserDatas(NULL)
    {
    }

    PxF32 chassisMass;
    PxVec3 chassisDims;
    PxVec3 chassisMOI;
    PxVec3 chassisCMOffset;
    PxMaterial* chassisMaterial;
    PxFilterData chassisSimFilterData;  //word0 = collide type, word1 = collide against types, word2 = PxPairFlags

    PxF32 wheelMass;
    PxF32 wheelWidth;
    PxF32 wheelRadius;
    PxF32 wheelMOI;
    PxMaterial* wheelMaterial;
    PxU32 numWheels;
    PxFilterData wheelSimFilterData;	//word0 = collide type, word1 = collide against types, word2 = PxPairFlags

    ActorUserData* actorUserData;
    ShapeUserData* shapeUserDatas;
};
class PhysXVehicleController
{
public:
    PxVehicleDrive4W* initVehiclePX(float x, float y, float z, float r, int id);
    void stepVehicles(float timestep, Kart* kart[], const int numKarts);

    VehicleDesc initVehicleDesc(PxMaterial* material, const PxFilterData& chassisSimFilterData, const PxFilterData& wheelSimFilterData, const PxU32 vehicleId);

    PxRigidDynamic* createVehicleActor(const PxVehicleChassisData& chassisData,
        PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData,
        PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData,
        PxPhysics& physics);


    PxConvexMesh* createChassisMesh(const PxVec3 dims, PxPhysics& physics, PxCooking& cooking);
    PxConvexMesh* createWheelMesh(const PxF32 width, const PxF32 radius, PxPhysics& physics, PxCooking& cooking);
    PxVehicleDrive4W* createVehicle4W(const VehicleDesc& vehicle4WDesc, PxPhysics* physics, PxCooking* cooking);

    void computeWheelCenterActorOffsets4W(const PxF32 wheelFrontZ, const PxF32 wheelRearZ, const PxVec3& chassisDims,
        const PxF32 wheelWidth, const PxF32 wheelRadius, const PxU32 numWheels, PxVec3* wheelCentreOffsets);

    void setupWheelsSimulationData(const PxF32 wheelMass, const PxF32 wheelMOI, const PxF32 wheelRadius, const PxF32 wheelWidth,
        const PxU32 numWheels, const PxVec3* wheelCenterActorOffsets,
        const PxVec3& chassisCMOffset, const PxF32 chassisMass,
        PxVehicleWheelsSimData* wheelsSimData);

    void configureUserData(PxVehicleWheels* vehicle, ActorUserData* actorUserData, ShapeUserData* shapeUserDatas);

    //Controls
    void step(float timestep, Kart* kart[], const int numKarts);

    void startAccelerateReverseMode(Kart* kart);
    void startTurnHardLeftMode(Kart* kart);
    void startHandbrakeTurnLeftMode(Kart* kart);
    void startHandbrakeTurnRightMode(Kart* kart);
    void startAccelerateForwardsMode(Kart* kart);
    void startTurnHardRightMode(Kart* kart);
    void startBrakeMode(Kart* kart);
    void releaseAllControls(Kart* kart);

};
