#include "Mesh.h"
#include "PxPhysicsAPI.h"
#include "foundation/PxAllocatorCallback.h"

#define SIMULATION_STEP 0.01666666666f // (1/60) 60 Frames per second
#define PX_RECORD_MEMORY_ALLOCATIONS true
#define PX_THREADS 2

struct PxMaterialInfo
{
	float staticFriction;
	float dynamicFriction;
	float restitution;

	inline bool operator ==(const PxMaterialInfo& right) const
	{
		return staticFriction == right.staticFriction && dynamicFriction == right.dynamicFriction && restitution == right.restitution;
	}
};

struct PxMaterialInfoHasher
{
	inline std::size_t operator()(const PxMaterialInfo& key) const noexcept
	{
		std::size_t result = hash(key.staticFriction);
		hashCombine(result, key.dynamicFriction);
		hashCombine(result, key.restitution);
		return result;
	};
};

enum RigidBodyType {STATIC, DYNAMIC, KINEMATIC};

struct RigidBody
{
	RigidBodyType type;
	Mesh* mesh;
	unsigned char maxVertices;
	physx::PxConvexMesh* pxMesh;
	PxMaterialInfo material;
	float density; // Unused for static rigid bodies
	physx::PxRigidActor* pxRigidBody; // Either static or dynamic rigid body
};

struct StaticRigidBodyCreateInfo
{
	Mesh* mesh;
	unsigned char maxVertices;
	PxMaterialInfo material;

	operator RigidBody()
	{
		RigidBody rigidBody;
		rigidBody.type = STATIC;
		rigidBody.mesh = mesh;
		rigidBody.maxVertices = maxVertices;
		rigidBody.material = material;
		return rigidBody;
	}
};

struct DynamicRigidBodyCreateInfo
{
	Mesh* mesh;
	unsigned char maxVertices;
	PxMaterialInfo material;
	float density;
	bool kinematic = false;

	operator RigidBody()
	{
		RigidBody rigidBody;
		if (kinematic)
			rigidBody.type = KINEMATIC;
		else
			rigidBody.type = DYNAMIC;
		rigidBody.mesh = mesh;
		rigidBody.maxVertices = maxVertices;
		rigidBody.material = material;
		rigidBody.density = density;
		return rigidBody;
	}
};

struct CharacterController
{
	float radius;
	float height;
	
	float maxStepHeight;
	float maxSlope;
	float inisibleWallHeight;
	float maxJumpHeight;
	
	float contactOffset;
	float cacheVolumeFactor;
	
	bool slide;

        unsigned int transformChangedCallbackIndex;
	
	PxMaterialInfo material;
	physx::PxController* pxController;
};

class PhysicsSystem
{
private:
	std::unordered_map<PxMaterialInfo, physx::PxMaterial*, PxMaterialInfoHasher> mMaterials;

	std::vector<EntityID> mStaticEntityIDs;
	std::vector<EntityID> mDynamicEntityIDs;
	std::vector<EntityID> mControllerEntityIDs;
	
	Composition mRigidBodyComposition;
	Composition mCharacterControllerComposition;
	
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Mesh>& mMeshManager = ComponentManager<Mesh>::instance();
	ComponentManager<RigidBody>& mRigidBodyManager = ComponentManager<RigidBody>::instance();
	ComponentManager<CharacterController>& mCharacterControllerManager = ComponentManager<CharacterController>::instance();
	
	physx::PxDefaultAllocator allocatorCallback;
	physx::PxDefaultErrorCallback errorCallback;
	physx::PxFoundation* foundation;
	physx::PxPvd* pvd;
	physx::PxPhysics* physics;
	physx::PxScene* scene;
	physx::PxCooking* cooking;
	physx::PxCpuDispatcher* cpuDispatcher;
	physx::PxPvdSceneClient* pvdSceneClient;
	physx::PxControllerManager* controllerManager;

	float accumulator = 0;

	PhysicsSystem();

public:
	static PhysicsSystem& instance();

	PhysicsSystem(const PhysicsSystem& copy) = delete;
	~PhysicsSystem();

	physx::PxMaterial* getMaterial(const PxMaterialInfo& materialInfo);

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void controllerComponentAdded(const Entity& entity);
	void controllerComponentRemoved(const Entity& entity);

        void controllerTransformChanged(const Transform& transform);

	void update(const float& delta);
};
