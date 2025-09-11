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
#include "PhysXKarting.h"

using namespace physx;
// declare variables
// general PhysX stuff, to be wrapped up into a class
PxDefaultAllocator      mDefaultAllocatorCallback;
PxDefaultErrorCallback  mDefaultErrorCallback;
PxDefaultCpuDispatcher* mDispatcher = NULL;
PxTolerancesScale       mToleranceScale;
PxFoundation*           mFoundation = NULL;
PxPhysics*              mPhysics = NULL;
PxScene*                mScene = NULL;
PxCooking*              gCooking = NULL;
PxMaterial*             mMaterial = NULL;
PxMaterial*             grassMaterial = NULL;
PxPvd*                   mPvd = NULL;


//Vehicle related globals
// The threshold values POINT_REJECT_ANGLE and NORMAL_REJECT_ANGLE can be tuned
// to modify the conditions under which wheel contact points are ignored or accepted.
const float POINT_REJECT_ANGLE = physx::PxPi / 4.0f;
const float  NORMAL_REJECT_ANGLE = physx::PxPi / 4.0f;
const float  MAX_ACCELERATION = 50000.0;
PxBatchQuery* gBatchQuery = NULL;
VehicleSceneQueryData* gVehicleSceneQueryData = NULL;
//The class WheelContactModifyCallback identifies and modifies contacts
//that involve a wheel.  Contacts that can be identified and managed by the suspension
//system are ignored.  Any contacts that remain are modified to account for the rotation
//speed of the wheel around the rolling axis.
WheelContactModifyCallback gWheelContactModifyCallback;
//Tire model friction for each combination of drivable surface type and tire type.defaultMaterial
PxVehicleDrivableSurfaceToTireFrictionPairs* gFrictionPairs = NULL;
PxU16 gNbQueryHitsPerWheel = 8;
#define BLOCKING_SWEEPS 1

//Box stack properties
float halfExtent = 0.5f;
const physx::PxU32 boxSize = 5;
const int S = (boxSize * (boxSize + 1.0)) / 2;
physx::PxRigidDynamic* body[S];
physx::PxPvdTransport* transport = nullptr;

PxConvexMesh* createWedgeMesh(const PxVec3 dims, PxPhysics& physics, PxCooking& cooking)
{
    const PxF32 x = dims.x * 0.5f;
    const PxF32 y = dims.y * 0.5f;
    const PxF32 z = dims.z * 0.5f;
    PxVec3 verts[6] =
    {
        PxVec3(x,y,-z),
        PxVec3(x,y,z),
        PxVec3(x,-y,z),
        PxVec3(x,-y,-z),
        PxVec3(-x,-y,z),
        PxVec3(-x,-y,-z)
    };
    return createConvexMesh(verts, 6, physics, cooking);
}

PhysXKarting::~PhysXKarting()
{
    gVehicleSceneQueryData->free(mDefaultAllocatorCallback);

    //free simulation
    physx::PxCloseVehicleSDK();
    mScene->release();
    mDispatcher->release();
    mMaterial->release();
    grassMaterial->release();
    gCooking->release();
    mPhysics->release();
    mPvd->disconnect();
    mPvd->release();
    transport->release();
    mFoundation->release();


    cout << "PXKarting called" << endl;
}

void PhysXKarting::CreateHeightField(ExtendedVertexStruct* terrain,int sizeWH,float scaleXZ,float scaleY)
//Create ground to drive on.
{

    physx::PxFilterData groundPlaneSimFilterData(COLLISION_FLAG_GROUND, COLLISION_FLAG_CHASSIS, 0, 0);

    //HeightField used for terrain
    int numCols = sizeWH;
    int numRows = sizeWH;
    int scale = 1;
    float offsetXY = -(scaleXZ * sizeWH) / 2.0f;
    //float scaleXZ = 2.0f;

    PxHeightFieldSample* samples = (PxHeightFieldSample*)malloc(sizeof(PxHeightFieldSample) * (numRows * numCols));

    for (int i = 0; i < numRows; i++)
    {
        for (int j = 0; j < numCols; j++)
        {
            samples[i * numRows + j].height = terrain[j * numRows + i].pos.y * 255.0f;
            samples[i * numRows + j].materialIndex0 = 0;
            samples[i * numRows + j].materialIndex1 = 0;
            samples[i * numRows + j].setTessFlag();
        }
    }
    PxHeightFieldDesc hfDesc;
    hfDesc.format = PxHeightFieldFormat::eS16_TM;
    hfDesc.nbColumns = numCols;
    hfDesc.nbRows = numRows;
    hfDesc.samples.data = samples;
    hfDesc.samples.stride = sizeof(PxHeightFieldSample);

    PxHeightField* aHeightField = gCooking->createHeightField(hfDesc, mPhysics->getPhysicsInsertionCallback());
    PxHeightFieldGeometry hfGeom(aHeightField, PxMeshGeometryFlags(), scaleY / 255.0f, ((1.0f / ((float)numRows - 1.0f)) * (float)numRows) * scaleXZ,
        ((1.0f / ((float)numCols - 1.0f)) * (float)numCols) * scaleXZ);
    physx::PxTransform th(physx::PxVec3(offsetXY, -0.1 - 0.23f, offsetXY));
    PxRigidStatic* aHeightFieldActor = mPhysics->createRigidStatic(th);
    PxMaterial* aMaterialArray[] = { mMaterial ,grassMaterial };
    PxShape* aHeightFieldShape = PxRigidActorExt::createExclusiveShape(*aHeightFieldActor,
        hfGeom, aMaterialArray, 2);

    //Set the query filter data of the ground plane so that the vehicle raycasts can hit the ground.
    PxFilterData qryFilterData;
    setupDrivableSurface(qryFilterData);
    aHeightFieldShape->setQueryFilterData(qryFilterData);
    //Set the simulation filter data of the ground plane so that it collides with the chassis of a vehicle but not the wheels.
    aHeightFieldShape->setSimulationFilterData(groundPlaneSimFilterData);
    mScene->addActor(*aHeightFieldActor);

    aHeightField->release();

    free(samples);
}


int PhysXKarting::initScenePX()
{
    // init physx
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocatorCallback, mDefaultErrorCallback);
    if (!mFoundation) throw("PxCreateFoundation failed!");
    mPvd = PxCreatePvd(*mFoundation);
    transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
    mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

    mToleranceScale.length = 1;        // typical length of an object
    mToleranceScale.speed = 10.0f;         // typical speed of an object, gravity*1s is a reasonable choice

    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, PxTolerancesScale(), true, mPvd);
    physx::PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
    // create CPU dispatcher
    mDispatcher = physx::PxDefaultCpuDispatcherCreate(3);
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = VehicleFilterShader;
    sceneDesc.contactModifyCallback = &gWheelContactModifyCallback;            //Enable contact modification

    mScene = mPhysics->createScene(sceneDesc);

    physx::PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
    if (pvdClient)
    {
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    // create simulation
    mMaterial = mPhysics->createMaterial(0.6f, 0.4f, 0.6f);
    grassMaterial = mPhysics->createMaterial(1.0f, 1.0f, 0.6f);

    gCooking = PxCreateCooking(PX_PHYSICS_VERSION, *mFoundation, PxCookingParams(PxTolerancesScale()));

    //--------------------------------------------------------------------------------
    // Vehicle Setup
    //--------------------------------------------------------------------------------
    {
        physx::PxInitVehicleSDK(*mPhysics);
        physx::PxVehicleSetBasisVectors(physx::PxVec3(0, 1, 0), physx::PxVec3(0, 0, 1));
        physx::PxVehicleSetUpdateMode(physx::PxVehicleUpdateMode::eVELOCITY_CHANGE);
        physx::PxVehicleSetSweepHitRejectionAngles(POINT_REJECT_ANGLE, NORMAL_REJECT_ANGLE);
        physx::PxVehicleSetMaxHitActorAcceleration(MAX_ACCELERATION);

        //Create the batched scene queries for the suspension sweeps.
        //Use the post-filter shader to reject hit shapes that overlap the swept wheel at the start pose of the sweep.
        physx::PxQueryHitType::Enum(*sceneQueryPreFilter)(physx::PxFilterData, physx::PxFilterData, const void*, physx::PxU32, PxHitFlags&);
        physx::PxQueryHitType::Enum(*sceneQueryPostFilter)(physx::PxFilterData, physx::PxFilterData, const void*, PxU32, const PxQueryHit&);
        #if BLOCKING_SWEEPS
                sceneQueryPreFilter = &WheelSceneQueryPreFilterBlocking;
                sceneQueryPostFilter = &WheelSceneQueryPostFilterBlocking;
        #else
                sceneQueryPreFilter = &WheelSceneQueryPreFilterNonBlocking;
                sceneQueryPostFilter = &WheelSceneQueryPostFilterNonBlocking;
        #endif 
        //Create the batched scene queries for the suspension raycasts.
        gVehicleSceneQueryData = VehicleSceneQueryData::allocate(NUM_VEHICLES, PX_MAX_NB_WHEELS, gNbQueryHitsPerWheel, NUM_VEHICLES, sceneQueryPreFilter, sceneQueryPostFilter, mDefaultAllocatorCallback);
        gBatchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *gVehicleSceneQueryData, mScene);
        //Create the friction table for each combination of tire and surface type.
        gFrictionPairs = createFrictionPairs(mMaterial, grassMaterial);
    }


    //Create boxes
    physx::PxTransform t(physx::PxVec3(0));
    int k = 0;
    for (physx::PxU32 i = 0; i < boxSize; i++) {
        for (physx::PxU32 j = 0; j < boxSize - i; j++) {
            physx::PxTransform localTm(physx::PxVec3(physx::PxReal(j * halfExtent * 2) - physx::PxReal(boxSize - i) * halfExtent, physx::PxReal(i * halfExtent * 2 + 2), 10.5));// - 30.0f * halfExtent
            body[k] = mPhysics->createRigidDynamic(t.transform(localTm));
            physx::PxShape* shape = PxRigidActorExt::createExclusiveShape(*body[k], physx::PxBoxGeometry(halfExtent, halfExtent, halfExtent), *mMaterial);
            PxFilterData simFilterData(COLLISION_FLAG_OBSTACLE, COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBSTACLE ,0, 0);
            shape->setSimulationFilterData(simFilterData);
            physx::PxRigidBodyExt::updateMassAndInertia(*body[k], 0.002f);
            mScene->addActor(*body[k]);
            k++;
        }
    }
    
}
void PhysXKarting::step(float timestep)
{
    //Scene update.
    mScene->simulate(timestep);
    mScene->fetchResults(true);
}

