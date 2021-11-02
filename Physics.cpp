#include "Physics.h"
#include "Transform.h"
#include "pvd\PxPvd.h"
#include <iostream>

PhysicsSystem::PhysicsSystem()
{
	mLastCursorPosition = mWindowManager.cursorPosition();

	mTransformManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
	mTransformManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));
	mTransformManager.subscribeAddEvent(std::bind(&PhysicsSystem::controllerComponentAdded, this, std::placeholders::_1));
	mTransformManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::controllerComponentRemoved, this, std::placeholders::_1));

	mRigidBodyManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
	mRigidBodyManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));

	mCharacterControllerManager.subscribeAddEvent(std::bind(&PhysicsSystem::controllerComponentAdded, this, std::placeholders::_1));
	mCharacterControllerManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::controllerComponentAdded, this, std::placeholders::_1));

	mRigidBodyComposition = mTransformManager.bit | mRigidBodyManager.bit;
	mCharacterControllerComposition = mTransformManager.bit | mCharacterControllerManager.bit;

	forwardSlerp.reverse();
	forwardSlerp.start();

	backSlerp.reverse();
	backSlerp.start();

	leftSlerp.reverse();
	leftSlerp.start();

	rightSlerp.reverse();
	rightSlerp.start();

	mWindowManager.subscribeKeyPressEvent(W, std::bind(&PhysicsSystem::forwardKey, this));
	mWindowManager.subscribeKeyPressEvent(S, std::bind(&PhysicsSystem::backKey, this));
	mWindowManager.subscribeKeyPressEvent(A, std::bind(&PhysicsSystem::leftKey, this));
	mWindowManager.subscribeKeyPressEvent(D, std::bind(&PhysicsSystem::rightKey, this));

	mWindowManager.subscribeKeyReleaseEvent(W, std::bind(&PhysicsSystem::forwardKey, this));
	mWindowManager.subscribeKeyReleaseEvent(S, std::bind(&PhysicsSystem::backKey, this));
	mWindowManager.subscribeKeyReleaseEvent(A, std::bind(&PhysicsSystem::leftKey, this));
	mWindowManager.subscribeKeyReleaseEvent(D, std::bind(&PhysicsSystem::rightKey, this));

	// PhysX Initialization
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

	assert(PxInitExtensions(*physics, pvd), "[ERROR] Physx extension initialization failed");

	// Create scene
	physx::PxSceneDesc sceneDesc(scale);
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
	PxCloseExtensions();
	for (const std::pair<PxMaterialInfo, physx::PxMaterial*>& material : mMaterials)
		material.second->release();
	controllerManager->release();
	cooking->release();
	scene->release();
	physics->release();
	pvd->release();
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

	if (staticIDIterator != mStaticEntityIDs.end())
	{
		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		rigidBody.pxMesh->release();
		rigidBody.pxRigidBody->release();
		mStaticEntityIDs.erase(staticIDIterator);
	}
	else if (dynamicIDIterator != mDynamicEntityIDs.end())
	{
		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		rigidBody.pxMesh->release();
		rigidBody.pxRigidBody->release();
		mDynamicEntityIDs.erase(dynamicIDIterator);
	}
}

void PhysicsSystem::controllerComponentAdded(const Entity& entity)
{
	if ((entity.composition() & mCharacterControllerComposition) == mCharacterControllerComposition)
	{
		Transform& transform = entity.getComponent<Transform>();
		CharacterController& characterController = entity.getComponent<CharacterController>();

		physx::PxCapsuleControllerDesc controllerDesc;
		controllerDesc.position = physx::PxExtendedVec3{ transform.position.x, transform.position.y, transform.position.z };
		controllerDesc.upDirection = { 0, 1, 0 };
		controllerDesc.slopeLimit = characterController.maxSlope;
		controllerDesc.invisibleWallHeight = characterController.invisibleWallHeight;
		controllerDesc.maxJumpHeight = characterController.maxJumpHeight;
		controllerDesc.contactOffset = characterController.contactOffset;
		controllerDesc.stepOffset = characterController.maxStepHeight;
		controllerDesc.volumeGrowth = characterController.cacheVolumeFactor;
		controllerDesc.reportCallback = nullptr;
		controllerDesc.behaviorCallback = nullptr;
		controllerDesc.nonWalkableMode = characterController.slide ? physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING_AND_FORCE_SLIDING : physx::PxControllerNonWalkableMode::ePREVENT_CLIMBING;
		controllerDesc.material = getMaterial(characterController.material);

		controllerDesc.radius = characterController.radius;
		controllerDesc.height = characterController.height;
		controllerDesc.climbingMode = physx::PxCapsuleClimbingMode::eEASY;

		characterController.pxController = controllerManager->createController(controllerDesc);

		mControllerEntityIDs.push_back(entity.ID());
	}
}

void PhysicsSystem::controllerComponentRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator controllerIDIterator = std::find(mControllerEntityIDs.begin(), mControllerEntityIDs.end(), entity.ID());
	if (controllerIDIterator != mControllerEntityIDs.end())
	{
		CharacterController& characterController = entity.getComponent<CharacterController>();
		characterController.pxController->release();
		mControllerEntityIDs.erase(controllerIDIterator);
	}
}

void PhysicsSystem::update(const float& deltaTime)
{
	accumulator += deltaTime;
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

		accumulator = 0.0f;
	}

	bool spaceDown = mWindowManager.keyDown(Space);

	glm::vec2 cursorPosition = mWindowManager.cursorPosition();
	glm::vec2 cursorDelta = cursorPosition - mLastCursorPosition;
	mLastCursorPosition = cursorPosition;

	// Input controls all CharacterControllers in the scene
	for (const EntityID& ID : mControllerEntityIDs)
	{
		Transform& transform = mTransformManager.getComponent(ID);
		CharacterController& controller = mCharacterControllerManager.getComponent(ID);

		transform.rotate(-cursorDelta.x * 0.005f, glm::vec3(0.0f, 1.0f, 0.0f));

		physx::PxRaycastBuffer hit;
		bool grounded = scene->raycast({ transform.position.x , transform.position.y - 0.5f * controller.height - controller.radius, transform.position.z }, { 0.0f, -1.0f, 0.0f }, 0.5f, hit, (physx::PxHitFlags)physx::PxHitFlag::eDEFAULT, physx::PxQueryFilterData(physx::PxQueryFlag::eSTATIC));

		if (grounded)
		{
			glm::vec3 xzVelocity(0.0f);
			xzVelocity += transform.direction() * forwardSlerp.sample();
			xzVelocity += transform.direction(glm::vec3(0.0f, 0.0f, 1.0f)) * backSlerp.sample();
			xzVelocity += transform.direction(glm::vec3(-1.0f, 0.0f, 0.0f)) * leftSlerp.sample();
			xzVelocity += transform.direction(glm::vec3(1.0f, 0.0f, 0.0f)) * rightSlerp.sample();
			float length = glm::length(xzVelocity);
			if (length > 1.0f)
				xzVelocity /= length;
			xzVelocity *= controller.speed;

			controller.velocity.x = xzVelocity.x;
			controller.velocity.z = xzVelocity.z;

			if (spaceDown)
				controller.velocity.y = controller.jumpSpeed;
			else
				controller.velocity.y = 0.0f;
		}
		else
			controller.velocity.y -= 9.8 * deltaTime;
		
		glm::vec3 displacement = controller.velocity * deltaTime;
		controller.pxController->move({ displacement.x, displacement.y, displacement.z }, 0.01f, deltaTime, 0);
		
		const physx::PxExtendedVec3 position = controller.pxController->getPosition();
		transform.position = glm::vec3(position.x, position.y, position.z);
	}
}

void PhysicsSystem::forwardKey()
{
	forwardSlerp.reverse();
}

void PhysicsSystem::backKey()
{
	backSlerp.reverse();
}

void PhysicsSystem::leftKey()
{
	leftSlerp.reverse();
}

void PhysicsSystem::rightKey()
{
	rightSlerp.reverse();
}
