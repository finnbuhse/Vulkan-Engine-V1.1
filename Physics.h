#pragma once
#include "Vulkan.h"
#include "Transform.h"
#include "WindowManager.h"
#include "Math.h"
#include "PxPhysicsAPI.h"
#include "foundation/PxAllocatorCallback.h"

#define PX_RECORD_MEMORY_ALLOCATIONS true
#define PX_THREADS 2

#define CHARACTER_ACCELERATION_TIME 0.5

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

enum RigidBodyType { UNDEFINED, STATIC, DYNAMIC, KINEMATIC };

struct RigidBody
{
	EntityID entityID;

	RigidBodyType type;

	glm::vec3* vertices;
	unsigned int nVertices;

	// Unused for rigid bodies with a convex mesh.
	unsigned int* indices;
	unsigned int nIndices;

	unsigned char nComputeVertices; // Unused for rigid bodies with a triangle mesh.

	PxMaterialInfo material;
	float density; // Unused for static rigid bodies.

	physx::PxBase* pxMesh;
	physx::PxRigidActor* pxRigidBody;

	void applyForce(const glm::vec3& force);

	void setTransform(const glm::vec3& position, const glm::quat& rotation = glm::vec3(0.0f));

	void setLinearVelocity(const glm::vec3& velocity);

	glm::vec3 getLinearVelocity() const;
};

template <>
std::vector<char> serialize(const RigidBody& rigidbody);

template <>
void deserialize(const std::vector<char>& vecData, RigidBody& write);

struct StaticRigidBodyCreateInfo
{
	/* Assign nullptr for null initialization. However it is assumed that pxRigidBody and pxMesh both point to real objects when the entity is freed.
	Mostly used for deserialization.
	*/
	glm::vec3* vertices; 
	
	unsigned int nVertices;

	unsigned int* indices; // Value ignored if nIndices == 0.
	unsigned int nIndices; // Assign 0 to use computed convex mesh, otherwise triangle mesh created with original vertices & indices.

	unsigned char nComputeVertices; // Value ignored if nIndices != 0.

	PxMaterialInfo material;

	operator RigidBody()
	{
		RigidBody rigidBody;
		rigidBody.type = STATIC;
		rigidBody.vertices = vertices;
		rigidBody.nVertices = nVertices;
		rigidBody.indices = indices;
		rigidBody.nIndices = nIndices;
		rigidBody.nComputeVertices = nComputeVertices;
		rigidBody.material = material;
		return rigidBody;
	}
};

struct DynamicRigidBodyCreateInfo
{
	/* Assign nullptr for null initialization. However it is assumed that pxRigidBody and pxMesh both point to real objects when the entity is freed.
	Mostly used for deserialization.
	*/
	glm::vec3* vertices;
	unsigned int nVertices;

	unsigned char nComputeVertices;

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
		rigidBody.vertices = vertices;
		rigidBody.nVertices = nVertices;
		rigidBody.indices = nullptr;
		rigidBody.nIndices = 0;
		rigidBody.nComputeVertices = nComputeVertices;
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

	void setLinearVelocity(const glm::vec3& velocity);

	glm::vec3 getLinearVelocity() const;
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
	template <typename T>
	friend std::vector<char> serialize(const T& component);

	template <typename T>
	friend void deserialize(const std::vector<char>& vecData, T& write);

	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<RigidBody>& mRigidBodyManager = ComponentManager<RigidBody>::instance();
	ComponentManager<CharacterController>& mCharacterControllerManager = ComponentManager<CharacterController>::instance();
	WindowManager& mWindowManager = WindowManager::instance();

	const ComponentAddedCallback mRigidBodyComponentAddedCallback = std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mRigidBodyComponentRemovedCallback = std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mControllerComponentAddedCallback = std::bind(&PhysicsSystem::controllerComponentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mControllerComponentRemovedCallback = std::bind(&PhysicsSystem::controllerComponentRemoved, this, std::placeholders::_1);

	const TransformChangedCallback mStaticTransformChangedCallback = std::bind(&PhysicsSystem::staticTransformChanged, this, std::placeholders::_1);

	std::unordered_map<PxMaterialInfo, physx::PxMaterial*, PxMaterialInfoHasher> mMaterials;
	std::vector<physx::PxCollection*> mCollections;

	std::vector<EntityID> mStaticEntityIDs;
	std::vector<EntityID> mDynamicEntityIDs;
	std::vector<EntityID> mControllerEntityIDs;

	Composition mRigidBodyComposition;
	Composition mCharacterControllerComposition;

	physx::PxDefaultAllocator allocatorCallback;
	physx::PxDefaultErrorCallback errorCallback;
	physx::PxFoundation* foundation;
	physx::PxPvd* pvd;
	physx::PxPvdTransport* transport;
	physx::PxPhysics* physics;
	physx::PxScene* scene;
	physx::PxCooking* cooking;
	physx::PxCpuDispatcher* cpuDispatcher;
	physx::PxPvdSceneClient* pvdSceneClient;
	physx::PxControllerManager* controllerManager;
	physx::PxSerializationRegistry* serializationRegistry;

	glm::vec2 mLastCursorPosition;

	double forwardDuration = 0.0;
	double rightDuration = 0.0;

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

	void update(const double& delta);

	void staticTransformChanged(const Transform& transform) const;

	physx::PxRaycastBuffer raycast(const glm::vec3& origin, const glm::vec3& direction, const float& distance, const RigidBodyType& filter = UNDEFINED) const;
};
