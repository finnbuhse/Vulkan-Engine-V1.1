#include "Physics.h"
#include "pvd\PxPvd.h"

template <>
std::vector<char> serialize(const RigidBody& rigidbody)
{
	static physx::PxSerializationRegistry* registry = PhysicsSystem::instance().getSerializationRegistry();

	std::vector<char> result;

	physx::PxCollection* collection = PxCreateCollection();
	
	if (rigidbody.type == STATIC)
		collection->add(*((physx::PxRigidStatic*)rigidbody.pxRigidBody));
	else
		collection->add(*((physx::PxRigidDynamic*)rigidbody.pxRigidBody));
	physx::PxSerialization::complete(*collection, *registry);

	physx::PxDefaultMemoryOutputStream writeBuffer;

	physx::PxSerialization::serializeCollectionToBinary(writeBuffer, *collection, *registry);

	std::vector<char> vecData(writeBuffer.getData(), writeBuffer.getData() + writeBuffer.getSize());
	result.insert(result.end(), vecData.begin(), vecData.end());
	return result;
}

std::vector<physx::PxCollection*> collections;

template <>
void deserialize(const std::vector<char>& vecData, RigidBody& write)
{
	static PhysicsSystem& physicsSystem = PhysicsSystem::instance();

	void* memory = malloc(vecData.size() + PX_SERIAL_FILE_ALIGN);
	void* memory128 = (void*)((size_t(memory) + PX_SERIAL_FILE_ALIGN) & ~(PX_SERIAL_FILE_ALIGN - 1));
	memcpy(memory128, vecData.data(), vecData.size());

	physx::PxCollection* collection = physx::PxSerialization::createCollectionFromBinary(memory128, *physicsSystem.getSerializationRegistry());
	collections.push_back(collection);

	write.pxMesh = &collection->getObject(0);
	write.pxRigidBody = (physx::PxRigidActor*)&collection->getObject(3);

	physicsSystem.getScene()->addCollection(*collection);
}

void releaseDeserializedCollections()
{
	for (physx::PxCollection* collection : collections)
	{
		collection->release();
	}
}

PhysicsSystem::PhysicsSystem()
{
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

	// Input
	mLastCursorPosition = mWindowManager.cursorPosition();

	forwardLerp.reverse();
	forwardLerp.start();

	backLerp.reverse();
	backLerp.start();

	leftLerp.reverse();
	leftLerp.start();

	rightLerp.reverse();
	rightLerp.start();

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
	assert((foundation, "[ERROR] PhysX foundation creation failed"));

	physx::PxTolerancesScale scale;
	scale.length = 100.0f;
	scale.speed = 981.0f;

	#ifdef NDEBUG
	pvd = nullptr;
	transport = nullptr;

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, scale, PX_RECORD_MEMORY_ALLOCATIONS, nullptr);
	assert((physics, "[ERROR] PhysX physics creation failed"));
	#else
	pvd = physx::PxCreatePvd(*foundation);
	transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, scale, PX_RECORD_MEMORY_ALLOCATIONS, pvd);
	assert((physics, "[ERROR] PhysX physics creation failed"));
	#endif

	cooking = PxCreateCooking(PX_PHYSICS_VERSION, *foundation, physx::PxCookingParams(scale));
	assert((cooking, "[ERROR] PhysX cooking creation failed"));

	bool status = PxInitExtensions(*physics, pvd);
	assert((status, "[ERROR] PhysX extension initialization failed"));
	
	// Create scene
	physx::PxSceneDesc sceneDesc(scale);
	sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = cpuDispatcher = physx::PxDefaultCpuDispatcherCreate(PX_THREADS);
	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
	scene = physics->createScene(sceneDesc);

	#ifdef NDEBUG
	pvdSceneClient = nullptr;
	#else
	pvdSceneClient = scene->getScenePvdClient();
	assert(pvdSceneClient, "[ERROR] PhysX pvd creation failed");
	pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
	pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
	pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	#endif
	controllerManager = PxCreateControllerManager(*scene);

	serializationRegistry = physx::PxSerialization::createSerializationRegistry(*physics);
}

PhysicsSystem::~PhysicsSystem()
{
	for (const std::pair<PxMaterialInfo, physx::PxMaterial*>& material : mMaterials)
		material.second->release();
	serializationRegistry->release();
	controllerManager->release();
	scene->release();
	PxCloseExtensions();
	cooking->release();
	physics->release();
	#ifndef NDEBUG
	transport->release();
	pvd->release();
	#endif
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
		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		rigidBody.entity = &entity;

		if (rigidBody.nVertices > 0)
		{
			Transform& transform = entity.getComponent<Transform>();
			const physx::PxTransform pxTransform(physx::PxVec3{ transform.position.x, transform.position.y, transform.position.z }, physx::PxQuat{ transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w });

			physx::PxGeometry* geometry;

			if (rigidBody.type == STATIC && rigidBody.nIndices > 0)
			{
				physx::PxTriangleMeshDesc meshDesc;
				meshDesc.points.count = rigidBody.nVertices;
				meshDesc.points.stride = sizeof(glm::vec3);
				meshDesc.points.data = rigidBody.vertices;
				meshDesc.triangles.count = rigidBody.nIndices;
				meshDesc.triangles.stride = 3 * sizeof(unsigned int);
				meshDesc.triangles.data = rigidBody.indices;

				assert((meshDesc.isValid(), "[ERROR PHYSX] Invalid mesh descriptor"));

				physx::PxDefaultMemoryOutputStream writeBuffer;
				physx::PxTriangleMeshCookingResult::Enum result;
				bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
				assert((status, "[ERROR PHYSX] Convex mesh cook failed"));
				assert((!result, "[ERROR PHYSX] Convex mesh cook failed"));

				physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
				rigidBody.pxMesh = physics->createTriangleMesh(readBuffer);

				geometry = new physx::PxTriangleMeshGeometry((physx::PxTriangleMesh*)rigidBody.pxMesh, physx::PxMeshScale({ transform.scale.x, transform.scale.y, transform.scale.z }));
			}
			else
			{
				physx::PxConvexMeshDesc meshDesc;
				meshDesc.points.count = rigidBody.nVertices;
				meshDesc.points.stride = sizeof(physx::PxVec3);
				meshDesc.points.data = rigidBody.vertices;
				meshDesc.vertexLimit = rigidBody.nComputeVertices;
				meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

				assert((meshDesc.isValid(), "[ERROR PHYSX] Invalid mesh descriptor"));

				physx::PxDefaultMemoryOutputStream writeBuffer;
				physx::PxConvexMeshCookingResult::Enum result;
				bool status = cooking->cookConvexMesh(meshDesc, writeBuffer, &result);
				assert((status, "[ERROR PHYSX] Convex mesh cook failed"));
				assert((!result, "[ERROR PHYSX] Convex mesh cook failed"));

				physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
				rigidBody.pxMesh = physics->createConvexMesh(readBuffer);

				geometry = new physx::PxConvexMeshGeometry((physx::PxConvexMesh*)rigidBody.pxMesh, physx::PxMeshScale({ transform.scale.x, transform.scale.y, transform.scale.z }));
			}

			if (rigidBody.type == STATIC)
			{
				rigidBody.pxRigidBody = physx::PxCreateStatic(*physics, pxTransform, *geometry, *getMaterial(rigidBody.material));
				assert((rigidBody.pxRigidBody, "[ERROR PHYSX] Rigid body creation failed"));
				mStaticEntityIDs.push_back(entity.ID());
			}
			else
			{
				rigidBody.pxRigidBody = physx::PxCreateDynamic(*physics, pxTransform, *geometry, *getMaterial(rigidBody.material), rigidBody.density);
				assert((rigidBody.pxRigidBody, "[ERROR PHYSX] Rigid body creation failed"));
				if (rigidBody.type == KINEMATIC)
					((physx::PxRigidDynamic*)rigidBody.pxRigidBody)->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
				mDynamicEntityIDs.push_back(entity.ID());
			}
			scene->addActor(*rigidBody.pxRigidBody);

			delete geometry;
		}
		else // Null initialization allowed but it is assumed that pxRigidBody and pxMesh point to valid PxRigidActor and PxBase objects when the component is removed
		{
			rigidBody.pxMesh = nullptr;
			rigidBody.pxRigidBody = nullptr;
			if (rigidBody.type == STATIC)
				mStaticEntityIDs.push_back(entity.ID());
			else
				mDynamicEntityIDs.push_back(entity.ID());
		}
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
	scene->simulate(deltaTime);
	scene->fetchResults(true);

	// Rigid bodies
	for (const EntityID& ID : mDynamicEntityIDs)
	{
		Transform& transform = mTransformManager.getComponent(ID);
		RigidBody& rigidBody = mRigidBodyManager.getComponent(ID);

		const physx::PxTransform pxTransform = rigidBody.pxRigidBody->getGlobalPose();
		transform.position = glm::vec3(pxTransform.p.x, pxTransform.p.y, pxTransform.p.z);
		transform.rotation = glm::quat(pxTransform.q.w, pxTransform.q.x, pxTransform.q.y, pxTransform.q.z);
	}

	// Character controllers
	bool spaceDown = mWindowManager.keyDown(Space);

	glm::vec2 cursorPosition = mWindowManager.cursorPosition();
	glm::vec2 cursorDelta = cursorPosition - mLastCursorPosition;
	mLastCursorPosition = cursorPosition;

	// Input controls all CharacterControllers
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
			xzVelocity += transform.direction() * forwardLerp.sample();
			xzVelocity += transform.direction(glm::vec3(0.0f, 0.0f, 1.0f)) * backLerp.sample();
			xzVelocity += transform.direction(glm::vec3(-1.0f, 0.0f, 0.0f)) * leftLerp.sample();
			xzVelocity += transform.direction(glm::vec3(1.0f, 0.0f, 0.0f)) * rightLerp.sample();
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
		physx::PxControllerCollisionFlags flags = controller.pxController->move({ displacement.x, displacement.y, displacement.z }, 0.01f, deltaTime, 0);

		physx::PxExtendedVec3 position = controller.pxController->getPosition();
		if (flags & physx::PxControllerCollisionFlag::eCOLLISION_UP)
		{
			controller.pxController->setPosition(position + physx::PxExtendedVec3(0.0f, -0.05f, 0.0f));
			controller.velocity.y = 0.0f;
		}
		
		position = controller.pxController->getPosition();
		transform.position = glm::vec3(position.x, position.y, position.z);
	}
}

void PhysicsSystem::forwardKey()
{
	forwardLerp.reverse();
}

void PhysicsSystem::backKey()
{
	backLerp.reverse();
}

void PhysicsSystem::leftKey()
{
	leftLerp.reverse();
}

void PhysicsSystem::rightKey()
{
	rightLerp.reverse();
}

physx::PxScene* PhysicsSystem::getScene()
{
	return scene;
}

physx::PxSerializationRegistry* PhysicsSystem::getSerializationRegistry()
{
	return serializationRegistry;
}
