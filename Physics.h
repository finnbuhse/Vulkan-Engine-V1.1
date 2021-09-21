#include "Mesh.h"
#include "PxPhysicsAPI.h"
#include "foundation/PxAllocatorCallback.h"

class PxAllocator : public physx::PxAllocatorCallback
{
public:
	void* allocate(size_t size, const char* typeName, const char* filename, int line) override;
	void deallocate(void* ptr) override;
};

class PxErrorHandler : public physx::PxErrorCallback
{
	void reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line) override;
};

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

struct RigidBody
{
	Mesh* mesh;
	PxMaterialInfo materialInfo;
	physx::PxMaterial* material;
};

struct RigidBodyStatic : public RigidBody
{
	physx::PxRigidStatic* pxRigidBody;
};

struct RigidBodyDynamic : public RigidBody
{
	float density;
	physx::PxRigidDynamic* pxRigidBody;
};

struct RigidBodyCreateInfo
{
	Mesh* mesh;
	PxMaterialInfo materialInfo;
	float density = 0.0f; // Not used in RigidBodyStatic

	operator RigidBodyStatic()
	{
		RigidBodyStatic rigidBody;
		rigidBody.mesh = mesh;
		rigidBody.materialInfo = materialInfo;
		return rigidBody;
	}

	operator RigidBodyDynamic()
	{
		RigidBodyDynamic rigidBody;
		rigidBody.mesh = mesh;
		rigidBody.materialInfo = materialInfo;
		rigidBody.density = density;
		return rigidBody;
	}
};

class PhysicsSystem
{
private:
	std::unordered_map<PxMaterialInfo, physx::PxMaterial*, PxMaterialInfoHasher> mMaterials;
	std::unordered_map<Mesh*, physx::PxShape*> mShapes;
	std::vector<EntityID> mEntityIDs;
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Mesh>& mMeshManager = ComponentManager<Mesh>::instance();
	ComponentManager<RigidBodyStatic>& mRigidStaticManager = ComponentManager<RigidBodyStatic>::instance();
	ComponentManager<RigidBodyDynamic>& mRigidDynamicManager = ComponentManager<RigidBodyDynamic>::instance();

	PxAllocator allocator;
	PxErrorHandler errorHandler;
	physx::PxFoundation* foundation;
	physx::PxPvd* pvd;
	physx::PxPhysics* physics;
	physx::PxScene* scene;
	physx::PxCooking* cooking;
	physx::PxCpuDispatcher* cpuDispatcher;
	physx::PxPvdSceneClient* pvdSceneClient;

	PhysicsSystem();

public:
	static PhysicsSystem& instance();

	PhysicsSystem(const PhysicsSystem& copy) = delete;
	~PhysicsSystem();

	physx::PxMaterial* getMaterial(const PxMaterialInfo& materialInfo);
	physx::PxShape* getShape(Mesh* mesh, physx::PxMaterial* material);

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void update(const float& delta);
};