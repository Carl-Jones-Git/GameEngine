#pragma once
#include <PxPhysicsAPI.h>
#include <PxFiltering.h>

//Contact modification values.
#define WHEEL_TANGENT_VELOCITY_MULTIPLIER 0.1f
#define MAX_IMPULSE PX_MAX_F32
using namespace physx;
enum
{
    DRIVABLE_SURFACE = 0xffff0000,
    UNDRIVABLE_SURFACE = 0x0000ffff
};

//Drivable surface types.
enum
{
    SURFACE_TYPE_TARMAC,
    SURFACE_TYPE_GRASS,
    MAX_NUM_SURFACE_TYPES
};

//Tire types.
enum
{
    TIRE_TYPE_NORMAL = 0,
    TIRE_TYPE_WORN,
    TIRE_TYPE_NORMAL_AI,
    TIRE_TYPE_WORN_AI,
    MAX_NUM_TIRE_TYPES
};


struct ActorUserData
{
    ActorUserData()
        : vehicle(NULL),
        actor(NULL)
    {
    }

    const PxVehicleWheels* vehicle;
    const PxActor* actor;
};

struct ShapeUserData
{
    ShapeUserData()
        : isWheel(false),
        wheelId(0xffffffff)
    {
    }

    bool isWheel;
    PxU32 wheelId;
};
PxTriangleMesh* createTriangleMesh(const PxVec3* verts, const PxU32 nbVerts, PxU32* indices32, const PxU32 triCount, PxPhysics& physics, PxCooking& cooking);
PxConvexMesh* createConvexMesh(const PxVec3* verts, const PxU32 numVerts, PxPhysics& physics, PxCooking& cooking);

PxFilterFlags VehicleFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);

PxQueryHitType::Enum WheelSceneQueryPreFilterNonBlocking(PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    PxHitFlags& queryFlags);

PxQueryHitType::Enum WheelSceneQueryPostFilterNonBlocking(PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    const PxQueryHit& hit); 

PxQueryHitType::Enum WheelSceneQueryPreFilterBlocking(PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    PxHitFlags& queryFlags);

PxQueryHitType::Enum WheelSceneQueryPostFilterBlocking(PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    const PxQueryHit& hit);

void setupDrivableSurface(PxFilterData& filterData);
void setupNonDrivableSurface(PxFilterData& filterData);
PxRigidStatic* createDrivablePlane(const PxFilterData& simFilterData, PxMaterial* material, PxPhysics* physics);
PxVehicleDrivableSurfaceToTireFrictionPairs* createFrictionPairs(const PxMaterial* defaultMaterial, const PxMaterial* grassMaterial);

//Data structure for quick setup of scene queries for suspension queries.
class VehicleSceneQueryData
{
public:
    VehicleSceneQueryData();
    ~VehicleSceneQueryData();

    //Allocate scene query data for up to maxNumVehicles and up to maxNumWheelsPerVehicle with numVehiclesInBatch per batch query.
    static VehicleSceneQueryData* allocate
    (const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, const PxU32 maxNumHitPointsPerWheel, const PxU32 numVehiclesInBatch,
        PxBatchQueryPreFilterShader preFilterShader, PxBatchQueryPostFilterShader postFilterShader,
        PxAllocatorCallback& allocator);

    //Free allocated buffers.
    void free(PxAllocatorCallback& allocator);

    //Create a PxBatchQuery instance that will be used for a single specified batch.
    static PxBatchQuery* setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene);

    //Return an array of scene query results for a single specified batch.
    PxRaycastQueryResult* getRaycastQueryResultBuffer(const PxU32 batchId);

    //Return an array of scene query results for a single specified batch.
    PxSweepQueryResult* getSweepQueryResultBuffer(const PxU32 batchId);

    //Get the number of scene query results that have been allocated for a single batch.
    PxU32 getQueryResultBufferSize() const;

private:

    //Number of queries per batch
    PxU32 mNumQueriesPerBatch;

    //Number of hit results per query
    PxU32 mNumHitResultsPerQuery;

    //One result for each wheel.
    PxRaycastQueryResult* mRaycastResults;
    PxSweepQueryResult* mSweepResults;

    //One hit for each wheel.
    PxRaycastHit* mRaycastHitBuffer;
    PxSweepHit* mSweepHitBuffer;

    //Filter shader used to filter drivable and non-drivable surfaces
    PxBatchQueryPreFilterShader mPreFilterShader;

    //Filter shader used to reject hit shapes that initially overlap sweeps.
    PxBatchQueryPostFilterShader mPostFilterShader;

};

class WheelContactModifyCallback : public PxContactModifyCallback
{
public:

    WheelContactModifyCallback()
        : PxContactModifyCallback()
    {
    }

    ~WheelContactModifyCallback() {}

    void onContactModify(PxContactModifyPair* const pairs, PxU32 count)
    {
        for (PxU32 i = 0; i < count; i++)
        {
            const PxRigidActor** actors = pairs[i].actor;
            const PxShape** shapes = pairs[i].shape;

            //Search for actors that represent vehicles and shapes that represent wheels.
            for (PxU32 j = 0; j < 2; j++)
            {
                const PxActor* actor = actors[j];
                if (actor->userData && (static_cast<ActorUserData*>(actor->userData))->vehicle)
                {
                    const PxVehicleWheels* vehicle = (static_cast<ActorUserData*>(actor->userData))->vehicle;
                    PX_ASSERT(vehicle->getRigidDynamicActor() == actors[j]);

                    const PxShape* shape = shapes[j];
                    if (shape->userData && (static_cast<ShapeUserData*>(shape->userData))->isWheel)
                    {
                        const PxU32 wheelId = (static_cast<ShapeUserData*>(shape->userData))->wheelId;
                        PX_ASSERT(wheelId < vehicle->mWheelsSimData.getNbWheels());

                        //Modify wheel contacts.
                        PxVehicleModifyWheelContacts(*vehicle, wheelId, WHEEL_TANGENT_VELOCITY_MULTIPLIER, MAX_IMPULSE, pairs[i]);
                    }
                }
            }
        }
    }

private:

};

