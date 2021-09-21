#include "Physics.h"
#include "Transform.h"
#include "pvd\PxPvd.h"
#include <iostream>

#define PX_RECORD_MEMORY_ALLOCATIONS true
#define PX_THREADS 2

void* PxAllocator::allocate(size_t size, const char* typeName, const char* filename, int line)
{
	return physx::platformAlignedAlloc(size);
}

void PxAllocator::deallocate(void* ptr)
{
	physx::platformAlignedFree(ptr);
}

void PxErrorHandler::reportError(physx::PxErrorCode::Enum code, const char* message, const char* file, int line)
{
	std::cout << "[PHYSX ERROR " << code << "] " << message << " Occurred in " << file << " line " << line << std::endl;
	assert(false);
}

PhysicsSystem::PhysicsSystem()
{
    mTransformManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
    mTransformManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));
	mMeshManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
	mMeshManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));
    mRigidStaticManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
    mRigidStaticManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));
	mRigidDynamicManager.subscribeAddEvent(std::bind(&PhysicsSystem::componentAdded, this, std::placeholders::_1));
	mRigidDynamicManager.subscribeRemoveEvent(std::bind(&PhysicsSystem::componentRemoved, this, std::placeholders::_1));

	// PhysX Initialization
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorHandler);
	assert(foundation, "[ERROR] PhysX foundation creation failed");

	pvd = physx::PxCreatePvd(*foundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("192.168.1.83", 5425, 10);
	pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	physx::PxTolerancesScale scale = physx::PxTolerancesScale();
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
	if (pvdSceneClient)
	{
		pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdSceneClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}
}

PhysicsSystem::~PhysicsSystem()
{
	for (const std::pair<Mesh*, physx::PxShape*>& shape : mShapes)
		shape.second->release();
	for (const std::pair<PxMaterialInfo, physx::PxMaterial*>& material : mMaterials)
		material.second->release();
	cooking->release();
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

physx::PxShape* PhysicsSystem::getShape(Mesh* mesh, physx::PxMaterial* material) // Note: Same mesh should have same material. Passed in for shape creation
{
	std::unordered_map<Mesh*, physx::PxShape*>::iterator shapeIterator = mShapes.find(mesh);
	if (shapeIterator != mShapes.end())
		return shapeIterator->second;

	physx::PxShape* shape;

	physx::PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = mesh->nVertices;
	meshDesc.points.stride = sizeof(physx::PxVec3);
	meshDesc.points.data = mesh->positions().data();

	meshDesc.triangles.count = mesh->nIndices / 3;
	meshDesc.triangles.stride = sizeof(unsigned int) * 3;
	meshDesc.triangles.data = mesh->indices;

	assert(meshDesc.isValid(), "[ERROR PHYSX] Invalid mesh descriptor");

	physx::PxDefaultMemoryOutputStream writeBuffer;
	physx::PxTriangleMeshCookingResult::Enum result;
	bool status = cooking->cookTriangleMesh(meshDesc, writeBuffer, &result);
	assert(status, "[ERROR PHYSX] Triangle mesh cook failed");
	assert(result, "[ERROR PHYSX] Triangle mesh cook failed");

	physx::PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());

	physx::PxTriangleMeshGeometry geometry(physics->createTriangleMesh(readBuffer));
	shape = physics->createShape(geometry, *material);

	mShapes.insert({ mesh, shape });

	return shape;
}

void PhysicsSystem::componentAdded(const Entity& entity)
{
	Composition entityComposition = entity.composition();
	if ((entityComposition & mTransformManager.bit) == mTransformManager.bit && (entityComposition & mMeshManager.bit) == mMeshManager.bit)
	{
		if ((entityComposition & mRigidStaticManager.bit) == mRigidStaticManager.bit)
		{
			Transform& transform = entity.getComponent<Transform>();
			RigidBodyStatic& rigidBody = entity.getComponent<RigidBodyStatic>();

			rigidBody.material = getMaterial(rigidBody.materialInfo);

			const physx::PxTransform pxTransform(physx::PxVec3{ transform.position.x, transform.position.y, transform.position.z }, physx::PxQuat{ transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w });
			rigidBody.pxRigidBody = physx::PxCreateStatic(*physics, pxTransform, *getShape(rigidBody.mesh, rigidBody.material));
			mEntityIDs.push_back(entity.ID());
		}
		else if ((entityComposition & mRigidDynamicManager.bit) == mRigidDynamicManager.bit)
		{
			Transform& transform = entity.getComponent<Transform>();
			RigidBodyDynamic& rigidBody = entity.getComponent<RigidBodyDynamic>();

			rigidBody.material = getMaterial(rigidBody.materialInfo);

			const physx::PxTransform pxTransform(physx::PxVec3{ transform.position.x, transform.position.y, transform.position.z }, physx::PxQuat{ transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w });
			rigidBody.pxRigidBody = physx::PxCreateDynamic(*physics, pxTransform, *getShape(rigidBody.mesh, rigidBody.material), rigidBody.density);
			//rigidBody.pxRigidBody->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC)
			mEntityIDs.push_back(entity.ID());
		}
	}
}

void PhysicsSystem::componentRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mEntityIDs.begin(), mEntityIDs.end(), entity.ID());
	if (IDIterator != mEntityIDs.end())
	{
		Composition entityComposition = entity.composition();
		if (entityComposition & mRigidStaticManager.bit)
			entity.getComponent<RigidBodyStatic>().pxRigidBody->release();
		else if (entityComposition & mRigidDynamicManager.bit)
			entity.getComponent<RigidBodyDynamic>().pxRigidBody->release();
		mEntityIDs.erase(IDIterator);
	}
}

void PhysicsSystem::update(const float& delta)
{

}