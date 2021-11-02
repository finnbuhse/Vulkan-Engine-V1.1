#include "Mesh.h"
#include "WindowManager.h"
#include "Math.h"
#include "PxPhysicsAPI.h"
#include "foundation/PxAllocatorCallback.h"

#define SIMULATION_STEP 0.013f
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

enum RigidBodyType { STATIC, DYNAMIC, KINEMATIC };

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

	float speed;
	float jumpSpeed;
	float maxStepHeight;
	float maxSlope;
	float invisibleWallHeight;
	float maxJumpHeight;

	float contactOffset;
	float cacheVolumeFactor;

	bool slide;

	PxMaterialInfo material;

	physx::PxController* pxController;

	glm::vec3 velocity;
};

struct CharacterControllerCreateInfo
{
	float radius;
	float height;

	float speed;
	float jumpSpeed;
	float maxStepHeight;
	float maxSlope;
	float invisibleWallHeight;
	float maxJumpHeight;

	float contactOffset;
	float cacheVolumeFactor;

	bool slide;

	PxMaterialInfo material;

	operator CharacterController()
	{
		CharacterController characterController;
		characterController.radius = radius;
		characterController.height = height;
		characterController.speed = speed;
		characterController.jumpSpeed = jumpSpeed;
		characterController.maxStepHeight = maxStepHeight;
		characterController.maxSlope = maxSlope;
		characterController.invisibleWallHeight = invisibleWallHeight;
		characterController.maxJumpHeight = maxJumpHeight;
		characterController.contactOffset = contactOffset;
		characterController.cacheVolumeFactor = cacheVolumeFactor;
		characterController.slide = slide;
		characterController.material = material;
		characterController.velocity = glm::vec3(0.0f);
		return characterController;
	}
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
	WindowManager& mWindowManager = WindowManager::instance();
	glm::vec2 mLastCursorPosition;

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

	LerpTime forwardSlerp = LerpTime(0.0f, 1.0f, 0.5f);
	LerpTime backSlerp = LerpTime(0.0f, 1.0f, 0.5f);
	LerpTime leftSlerp = LerpTime(0.0f, 1.0f, 0.5f);
	LerpTime rightSlerp = LerpTime(0.0f, 1.0f, 0.5f);

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

	void update(const float& delta);

	void forwardKey();
	void backKey();
	void leftKey();
	void rightKey();
};