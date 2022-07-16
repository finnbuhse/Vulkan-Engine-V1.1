#include "Physics.h"
#include "pvd\PxPvd.h"

void RigidBody::applyForce(const glm::vec3& force)
{
	if (type == DYNAMIC)
		((physx::PxRigidDynamic*)pxRigidBody)->addForce(physx::PxVec3(force.x, force.y, force.z));
}

void RigidBody::setTransform(const glm::vec3& position, const glm::quat& rotation)
{
	pxRigidBody->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
	Transform& transform = Entity(entityID).getComponent<Transform>();
	transform.position = position;
	transform.rotation = rotation;
}

void RigidBody::setLinearVelocity(const glm::vec3& velocity)
{
	if (type == DYNAMIC || type == KINEMATIC)
		((physx::PxRigidDynamic*)pxRigidBody)->setLinearVelocity(physx::PxVec3(velocity.x, velocity.y, velocity.z));
}

glm::vec3 RigidBody::getLinearVelocity() const
{
	if (type == DYNAMIC || type == KINEMATIC)
	{
		physx::PxVec3 pxVelocity = ((physx::PxRigidDynamic*)pxRigidBody)->getLinearVelocity();
		return glm::vec3(pxVelocity.x, pxVelocity.y, pxVelocity.z);
	}
	return glm::vec3(0.0f); // Static rigidbody
}

void CharacterController::setLinearVelocity(const glm::vec3& velocity)
{
	pxController->getActor()->setLinearVelocity(physx::PxVec3(velocity.x, velocity.y, velocity.z));
}

glm::vec3 CharacterController::getLinearVelocity() const
{
	physx::PxVec3 pxVelocity = pxController->getActor()->getLinearVelocity();
	return glm::vec3(pxVelocity.x, pxVelocity.y, pxVelocity.z);
}

template <>
std::vector<char> serialize(const RigidBody& rigidbody)
{
	static physx::PxSerializationRegistry* registry = PhysicsSystem::instance().serializationRegistry;

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

template <>
void deserialize(const std::vector<char>& vecData, RigidBody& write)
{
	static PhysicsSystem& physicsSystem = PhysicsSystem::instance();

	const Composition& composition = Entity::getCompositionFromID(write.entityID);
	assert(((composition & physicsSystem.mRigidBodyComposition) == physicsSystem.mRigidBodyComposition, "[ERROR] Attempting to deserialize a rigid body attached to an entity which does not possess all the required components"));
	assert((write.type == UNDEFINED, "[ERROR] Attempting to deserialize a rigid body which has already been initialized"));

	void* memory = malloc(vecData.size() + PX_SERIAL_FILE_ALIGN);
	void* memory128 = (void*)((size_t(memory) + PX_SERIAL_FILE_ALIGN) & ~(PX_SERIAL_FILE_ALIGN - 1));
	memcpy(memory128, vecData.data(), vecData.size());

	physx::PxCollection* collection = physx::PxSerialization::createCollectionFromBinary(memory128, *physicsSystem.serializationRegistry);
	physicsSystem.mCollections.push_back(collection);

	physicsSystem.scene->addCollection(*collection);

	write.pxMesh = &collection->getObject(0);
	write.pxRigidBody = (physx::PxRigidActor*)&collection->getObject(3);
	collection->remove(*write.pxMesh);
	collection->remove(*write.pxRigidBody);

	write.pxRigidBody->userData = new unsigned int(write.entityID);

	if (write.pxRigidBody->getConcreteType() == physx::PxConcreteType::eRIGID_STATIC)
	{
		write.type = STATIC;
		physicsSystem.mTransformManager.getComponent(write.entityID).subscribeChangedEvent(&physicsSystem.mStaticTransformChangedCallback);
		physicsSystem.mStaticEntityIDs.push_back(write.entityID);
	}
	else
	{
		if (((physx::PxRigidDynamic*)write.pxRigidBody)->getRigidBodyFlags() & physx::PxRigidBodyFlag::eKINEMATIC)
			write.type = KINEMATIC;
		else
			write.type = DYNAMIC;
		physicsSystem.mDynamicEntityIDs.push_back(write.entityID);
	}
}

PhysicsSystem::PhysicsSystem()
{
	mTransformManager.subscribeAddedEvent(&mRigidBodyComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mRigidBodyComponentRemovedCallback);
	mTransformManager.subscribeAddedEvent(&mControllerComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mControllerComponentRemovedCallback);

	mRigidBodyManager.subscribeAddedEvent(&mRigidBodyComponentAddedCallback);
	mRigidBodyManager.subscribeRemovedEvent(&mRigidBodyComponentRemovedCallback);

	mCharacterControllerManager.subscribeAddedEvent(&mControllerComponentAddedCallback);
	mCharacterControllerManager.subscribeRemovedEvent(&mControllerComponentRemovedCallback);

	mRigidBodyComposition = mTransformManager.bit | mRigidBodyManager.bit;
	mCharacterControllerComposition = mTransformManager.bit | mCharacterControllerManager.bit;

	// Input
	mLastCursorPosition = mWindowManager.cursorPosition();

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

PhysicsSystem& PhysicsSystem::instance()
{
	static PhysicsSystem instance;
	return instance;
}

PhysicsSystem::~PhysicsSystem()
{
	mTransformManager.unsubscribeAddedEvent(&mRigidBodyComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mRigidBodyComponentRemovedCallback);
	mTransformManager.unsubscribeAddedEvent(&mControllerComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mControllerComponentRemovedCallback);

	mRigidBodyManager.unsubscribeAddedEvent(&mRigidBodyComponentAddedCallback);
	mRigidBodyManager.unsubscribeRemovedEvent(&mRigidBodyComponentRemovedCallback);

	mCharacterControllerManager.unsubscribeAddedEvent(&mControllerComponentAddedCallback);
	mCharacterControllerManager.unsubscribeRemovedEvent(&mControllerComponentRemovedCallback);

	for (physx::PxCollection* collection : mCollections)
	{
		for (physx::PxU32 i = 0; i < collection->getNbObjects(); i++)
		{
			physx::PxBase& object = collection->getObject(i);
			switch (object.getConcreteType())
			{
			case physx::PxConcreteType::eSHAPE:
				static_cast<physx::PxShape&>(object).release();
				break;
			case physx::PxConcreteType::eMATERIAL:
				static_cast<physx::PxMaterial&>(object).release();
				break;
			}
		}
		collection->release();
	}

	for (const std::pair<PxMaterialInfo, physx::PxMaterial*>& material : mMaterials)
		material.second->release();
	serializationRegistry->release();
	controllerManager->release();
	scene->release();
	PxCloseExtensions();
	cooking->release();
	physics->release();
	#ifndef NDEBUG
	pvd->release();
	transport->release();
	#endif
	foundation->release();
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
		rigidBody.entityID = entity.ID();

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
				rigidBody.pxRigidBody->userData = new unsigned int(entity.ID());
				assert((rigidBody.pxRigidBody, "[ERROR PHYSX] Rigid body creation failed"));
				entity.getComponent<Transform>().subscribeChangedEvent(&mStaticTransformChangedCallback);
				mStaticEntityIDs.push_back(entity.ID());
			}
			else
			{
				rigidBody.pxRigidBody = physx::PxCreateDynamic(*physics, pxTransform, *geometry, *getMaterial(rigidBody.material), rigidBody.density);
				rigidBody.pxRigidBody->userData = new unsigned int(entity.ID());
				assert((rigidBody.pxRigidBody, "[ERROR PHYSX] Rigid body creation failed"));
				if (rigidBody.type == KINEMATIC)
					((physx::PxRigidDynamic*)rigidBody.pxRigidBody)->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);
				else
				{
					((physx::PxRigidDynamic*)rigidBody.pxRigidBody)->setSleepThreshold(0.1f);
					mDynamicEntityIDs.push_back(entity.ID());
				}
				
			}
			scene->addActor(*rigidBody.pxRigidBody);

			delete geometry;
		}
		else
		{
			rigidBody.type = UNDEFINED;
			rigidBody.pxMesh = nullptr;
			rigidBody.pxRigidBody = nullptr;
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
		delete rigidBody.pxRigidBody->userData;
		rigidBody.pxRigidBody->release();

		Transform& transform = entity.getComponent<Transform>();
		if (transform.changedCallbacks.data)
			transform.unsubscribeChangedEvent(&mStaticTransformChangedCallback);

		mStaticEntityIDs.erase(staticIDIterator);
	}
	else if (dynamicIDIterator != mDynamicEntityIDs.end())
	{
		RigidBody& rigidBody = entity.getComponent<RigidBody>();
		rigidBody.pxMesh->release();
		delete rigidBody.pxRigidBody->userData;
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

		physx::PxRigidDynamic* rigidDynamic = characterController.pxController->getActor();

		unsigned int bufferLength = rigidDynamic->getNbShapes();
		physx::PxShape** shapes = new physx::PxShape*[bufferLength];
		rigidDynamic->getShapes(shapes, bufferLength * sizeof(physx::PxShape*));
		shapes[0]->setFlag(physx::PxShapeFlag::eSIMULATION_SHAPE, false);
		shapes[0]->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);

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

void PhysicsSystem::update(const double& deltaTime)
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
	bool forwardDown = mWindowManager.keyDown(W);
	bool backwardDown = mWindowManager.keyDown(S);
	bool rightDown = mWindowManager.keyDown(D);
	bool leftDown = mWindowManager.keyDown(A);
	bool spaceDown = mWindowManager.keyDown(Space);

	if ((forwardDuration < -deltaTime && !backwardDown) | forwardDown)
		forwardDuration += deltaTime;
	else if (forwardDuration > deltaTime | backwardDown )
		forwardDuration -= deltaTime;
	/*
	else if (glm::abs(forwardDuration) < deltaTime)
		forwardDuration = 0.0;
	*/
	forwardDuration = glm::clamp(forwardDuration, -CHARACTER_ACCELERATION_TIME, CHARACTER_ACCELERATION_TIME);

	if ((rightDuration < -deltaTime && !leftDown) | rightDown)
		rightDuration += deltaTime;
	else if (rightDuration > deltaTime | leftDown)
		rightDuration -= deltaTime;
	/*
	else if (glm::abs(rightDuration) < deltaTime)
		rightDuration = 0.0;
	*/
	rightDuration = glm::clamp(rightDuration, -CHARACTER_ACCELERATION_TIME, CHARACTER_ACCELERATION_TIME);

	glm::vec2 cursorPosition = mWindowManager.cursorPosition();
	glm::vec2 cursorDelta = cursorPosition - mLastCursorPosition;
	mLastCursorPosition = cursorPosition;

	// Input controls all CharacterControllers
	for (const EntityID& ID : mControllerEntityIDs)
	{
		Transform& transform = mTransformManager.getComponent(ID);
		CharacterController& controller = mCharacterControllerManager.getComponent(ID);

		// Yaw character with cursor x axis
		transform.rotate(-cursorDelta.x * 0.005f, glm::vec3(0.0f, 1.0f, 0.0f));

		bool grounded = raycast(glm::vec3(transform.position.x, transform.position.y - 0.5 * controller.height - controller.radius, transform.position.z), glm::vec3(0.0f, -1.0f, 0.0f), 0.02f, UNDEFINED).hasBlock;
		
		if (grounded)
		{
			// Calculate global move velocity.
			glm::vec3 xzVelocity(lerp(0.0, 1.0, rightDuration / CHARACTER_ACCELERATION_TIME), 0.0f, -lerp(0.0, 1.0, forwardDuration / CHARACTER_ACCELERATION_TIME));
			xzVelocity = transform.rotation * xzVelocity;
			float length = glm::length(xzVelocity);
			if (length > 1.0f)
				xzVelocity /= length;
			xzVelocity *= controller.speed;

			controller.velocity.x = xzVelocity.x;
			controller.velocity.z = xzVelocity.z;

			if (spaceDown)
				controller.velocity.y = controller.jumpSpeed * deltaTime;
			else
				controller.velocity.y = 0.0;
		}
		else
			controller.velocity.y -= 9.8 * deltaTime;

		glm::vec3 displacement = controller.velocity * (float)deltaTime;
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

void PhysicsSystem::staticTransformChanged(const Transform& transform) const
{
	mRigidBodyManager.getComponent(transform.entityID).pxRigidBody->setGlobalPose(physx::PxTransform(physx::PxVec3(transform.position.x, transform.position.y, transform.position.z), physx::PxQuat(transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w)));
}

physx::PxRaycastBuffer PhysicsSystem::raycast(const glm::vec3& origin, const glm::vec3& direction, const float& distance, const RigidBodyType& filter) const
{
	physx::PxQueryFilterData filterData(physx::PxQueryFlag::Enum(0));
	switch (filter)
	{
	case UNDEFINED:
		filterData.flags |= physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC;
	case STATIC:
		filterData.flags |= physx::PxQueryFlag::eSTATIC;
		break;
	case KINEMATIC:
	case DYNAMIC:
		filterData.flags |= physx::PxQueryFlag::eDYNAMIC;
		break;
	}

	physx::PxRaycastBuffer hit;
	scene->raycast(physx::PxVec3(origin.x, origin.y, origin.z), physx::PxVec3(direction.x, direction.y, direction.z), distance, hit, physx::PxHitFlag::eDEFAULT | physx::PxHitFlag::eMODIFIABLE_FLAGS, filterData);
	return hit;
}
