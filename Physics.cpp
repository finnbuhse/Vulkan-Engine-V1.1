#include "Physics.h"
#include "Transform.h"
#include "pvd\PxPvd.h"
#include <iostream>

PhysicsSystem::PhysicsSystem()
{
    mTransformManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
    mTransformManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));
	mMeshManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
	mMeshManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));
    mRigidBodyManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
    mRigidBodyManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));

	mRigidBodyComposition = mTransformManager.bit | mRigidBodyManager.bit;
	mCharacterControllerComposition = mTransformManager.bit | mCharacterControllerManager.bit;

	// PhysX Initialization
	physx::PxDefaultAllocator();

	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocatorCallback, errorCallback);
	assert(foundation, "[ERROR] PhysX foundation creation failed");

	pvd = physx::PxCreatePvd(*foundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	physx::PxTolerancesScale scale;
	scale.length = 100.0f;
	scale.speed = 981.0f;

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, scale, PX_RECORD_MEMORY_ALLOCATIONS, pvd);
	assert(physics, "[ERROR] PhysX physics creation failed");

	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, physx::PxCookingParams(scale));
	assert(cooking, "[ERROR] PhysX cooking creation failed");

	// Create scene
	physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(PX_THREADS);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	scene = physics->createScene(sceneDesc);

	pvdSceneClient = scene->getScenePvdClient();
	assert(pvdSceneClient, "[ERROR] PhysX pvd creation failed");
	pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
	pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
	pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);

	controllerManager = PxCreateControllerManager(*scene);
}

PhysicsSystem::~PhysicsSystem()
{
	for (const std::pair<PxMaterialInfo, physx::PxMaterial*>& material : mMaterials)
		material.second->release();
	controllerManager->release();
	pvd->release();
	cooking->release();
	scene->release();
	physics->release();
	foundation->release();
}

PhysicsSystem& PhysicsSystem::instance()
{
    static PhysicsSystem instance;
    return instance;
}

physx::PxMaterial* PhysicsSystem::getMaterial(const PxMaterialInfo& materialInfo)
{
	std::unordered_map<PxMaterialInfo, physx::PxMaterial*, PxMaterialInfoHasher>::iterator it = mMaterials.find(materialInfo);
	if (it != mMaterials.end())
		return it->second;
	physx::PxMaterial* material = physics->createMaterial(materialInfo.staticFriction, materialInfo.dynamicFriction, materialInfo.restitution);
	mMaterials.insert({ materialInfo, material });
	return material;
}

void PhysicsSystem::componentAdded(const Entity& entity)
{ 
	if ((entity.composition() & mRigidBodyComposition) == mRigidBodyComposition)
	{
		Transform& transform = entity.getComponent<Transform>();
		const physx::PxTransform pxTransform(physx::PxVec3{ transform.position.x, transform.position.y, transform.position.z }, physx::PxQuat{ transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w });

		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		
		std::vector<glm::vec3> positions = rigidBody.mesh->positions();

		physx::PxConvexMeshDesc meshDesc;
		meshDesc.points.count = rigidBody.mesh->nVertices;
		meshDesc.points.stride = sizeof(physx::PxVec3);
		meshDesc.points.data = positions.data();
		meshDesc.vertexLimit = rigidBody.maxVertices;
		meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

		assert(meshDesc.isValid(), "[ERROR PHYSX] Invalid mesh descriptor");

		physx::PxDefaultMemoryOutputStream writeBuffer;
		physx::PxConvexMeshCookingResult::Enum result;
		bool status = cooking->cookConvexMesh(meshDesc, writeBuffer, &result);
		assert(status, "[ERROR PHYSX] Convex mesh cook failed");
		assert(!result, "[ERROR PHYSX] Convex mesh cook failed");

		physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
		rigidBody.pxMesh = physics->createConvexMesh(readBuffer);
		
		physx::PxConvexMeshGeometry geometry(rigidBody.pxMesh, physx::PxMeshScale({ transform.scale.x, transform.scale.y, transform.scale.z }));

		if (rigidBody.type == STATIC)
		{
			rigidBody.pxRigidBody = physx::PxCreateStatic(*physics, pxTransform, geometry, *getMaterial(rigidBody.material));
			assert(rigidBody.pxRigidBody, "[ERROR PHYSX] Rigid body creation failed");
			mStaticEntityIDs.push_back(entity.ID());
		}
		else
		{
			rigidBody.pxRigidBody = physx::PxCreateDynamic(*physics, pxTransform, geometry, *getMaterial(rigidBody.material), rigidBody.density);
			assert(rigidBody.pxRigidBody, "[ERROR PHYSX] Rigid body creation failed");
			if (rigidBody.type == KINEMATIC)
				((physx::PxRigidDynamic*)rigidBody.pxRigidBody)->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
			mDynamicEntityIDs.push_back(entity.ID());
		}
		scene->addActor(*rigidBody.pxRigidBody);
	}
}

void PhysicsSystem::componentRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator staticIDIterator = std::find(mStaticEntityIDs.begin(), mStaticEntityIDs.end(), entity.ID());
	std::vector<EntityID>::iterator dynamicIDIterator = std::find(mDynamicEntityIDs.begin(), mDynamicEntityIDs.end(), entity.ID());
	
	if (dynamicIDIterator != mDynamicEntityIDs.end())
	{
		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		rigidBody.pxMesh->release();
		rigidBody.pxRigidBody->release();
		mDynamicEntityIDs.erase(dynamicIDIterator);
	}
	else if (staticIDIterator != mStaticEntityIDs.end())
	{
		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		rigidBody.pxMesh->release();
		rigidBody.pxRigidBody->release();
		mStaticEntityIDs.erase(dynamicIDIterator);
	}
}

void PhysicsSystem::controllerComponentAdded(const Entity& entity)
{
	if ((entity.composition() & mCharacterControllerComposition) == mCharacterControllerComposition)
	{
		Transform& transform = entity.getComponent<Transform>();

		physx::PxCapsuleControllerDesc controllerDesc;
		controllerDesc.position = physx::PxExtendedVec3{ transform.position.x, transform.position.y, transform.position.z };
		controllerDesc.upDirection = { 0, 1, 0 };
		controllerDesc.slopeLimit = 0.71f;
		controllerDesc.invisibleWallHeight = 0.0f;
		controllerDesc.maxJumpHeight = 3.0f;
		controllerDesc.contactOffset = 0.1f;
		controllerDesc.stepOffset = 0.5f;
		controllerDesc.volumeGrowth = 1.5f;
		controllerDesc.reportCallback = nullptr;
		controllerDesc.behaviorCallback = nullptr;
		controllerDesc.nonWalkableMode = physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING;
		controllerDesc.material = getMaterial();

		controllerDesc.radius = 0.5f;
		controllerDesc.height = 4.0f;
		controllerDesc.climbingMode = physx::PxCapsuleClimbingMode::eEASY;
	}
}

void PhysicsSystem::update(const float& delta)
{
	accumulator += delta;
	if (accumulator >= SIMULATION_STEP)
	{
		scene->simulate(SIMULATION_STEP);
		scene->fetchResults(true);

		for (const EntityID& ID : mDynamicEntityIDs)
		{
			Transform& transform = mTransformManager.getComponent(ID);
			RigidBody& rigidBody = mRigidBodyManager.getComponent(ID);

			const physx::PxTransform pxTransform = rigidBody.pxRigidBody->getGlobalPose();
			transform.position = glm::vec3(pxTransform.p.x, pxTransform.p.y, pxTransform.p.z);
			transform.rotation = glm::quat(pxTransform.q.w, pxTransform.q.x, pxTransform.q.y, pxTransform.q.z);
		}
	}
}