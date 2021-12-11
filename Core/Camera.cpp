#include "Camera.h"

CameraCreateInfo::operator Camera() const
{
	Camera camera;
	camera.lastFov = 0.0f;
	camera.lastAspect = 0.0f;
	camera.lastZFar = 0.0f;
	camera.lastZNear = 0.0f;
	camera.fov = fov;
	camera.aspect = aspect;
	camera.zNear = zNear;
	camera.zFar = zFar;
	return camera;
}

void Camera::subscribeProjectionChangedEvent(const std::function<void(const Camera&)>* callback)
{
	projectionChangedCallbacks.push((std::function<void(const Camera&)>*)callback);
}

void Camera::unsubscribeProjectionChangedEvent(const std::function<void(const Camera&)>* callback)
{
	projectionChangedCallbacks.remove(projectionChangedCallbacks.find((std::function<void(const Camera&)>*)callback));
}

void Camera::subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>* callback)
{
	viewChangedCallbacks.push((std::function<void(const Transform&, const Camera&)>*)callback);
}

void Camera::unsubscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>* callback)
{
	viewChangedCallbacks.remove(viewChangedCallbacks.find((std::function<void(const Transform&, const Camera&)>*)callback));
}

template<>
std::vector<char> serialize(const Camera& camera)
{
	CameraCreateInfo serializeInfo;
	serializeInfo.fov = camera.fov;
	serializeInfo.aspect = camera.aspect;
	serializeInfo.zNear = camera.zNear;
	serializeInfo.zFar = camera.zFar;
	return serialize(serializeInfo);
}

template<>
void deserialize(const std::vector<char>& vecData, Camera& write)
{
	CameraCreateInfo createInfo;
	deserialize(vecData, createInfo);
	write = createInfo;
}

CameraSystem::CameraSystem()
{
	mComposition = mTransformManager.bit | mCameraManager.bit; // Entities must possess both a Transform and Camera component in order to be added to the system

	// Subscribe to component added and removed events so the entities can be added and removed from the system
	mTransformManager.subscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mCameraManager.subscribeAddedEvent(&mComponentAddedCallback);
	mCameraManager.subscribeRemovedEvent(&mComponentRemovedCallback);
}

CameraSystem& CameraSystem::instance()
{
	static CameraSystem instance;
	return instance;
}

CameraSystem::~CameraSystem()
{
	mTransformManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);
	mCameraManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mCameraManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);
}

void CameraSystem::componentAdded(const Entity& entity)
{
	if ((entity.composition() & mComposition) == mComposition) // Check if all required components have been added
	{
		Camera& camera = entity.getComponent<Camera>();

		// Construct camera
		camera.projectionChangedCallbacks.initialize();
		camera.viewChangedCallbacks.initialize();
		entity.getComponent<Transform>().subscribeChangedEvent(&mTransformChangedCallback);

		mEntityIDs.push_back(entity.ID()); // Add the entity to the system
	}
}

void CameraSystem::componentRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mEntityIDs.begin(), mEntityIDs.end(), entity.ID());
	if (IDIterator != mEntityIDs.end()) // Check if the entity is in the system
	{
		Camera& camera = entity.getComponent<Camera>();

		// Free cameras callback arrays
		camera.projectionChangedCallbacks.free();
		camera.viewChangedCallbacks.free();

		Transform& transform = entity.getComponent<Transform>();
		if(transform.changedCallbacks.data) // Incase the transform's data has already been freed
			transform.unsubscribeChangedEvent(&mTransformChangedCallback);

		mEntityIDs.erase(IDIterator); // Remove entity from the system
	}
}

void CameraSystem::transformChanged(Transform& transform) const
{
	Camera& camera = mCameraManager.getComponent(transform.entityID);
	 
	/* The view matrix is dependent on position and rotation and so must be updated once either of these change.
       It is possible to check for the type of change (See Transform.h) however here if the transform has changed; it is assumed to always be a translation or rotation
       since it does not make sense to enlarge a camera */
	camera.viewMatrix = glm::lookAt(transform.worldPosition, transform.worldPosition + transform.worldDirection(), glm::vec3(0.0f, 1.0f, 0.0f));

	// Invoke view changed callbacks
	for (unsigned int i = 0; i < camera.viewChangedCallbacks.length; i++)
		(*camera.viewChangedCallbacks[i])(transform, camera);
}

void CameraSystem::update() const
{
	// Detect change in projection parameters and update projection matrix accordingly
	for (const EntityID& ID : mEntityIDs)
	{
		Camera& camera = mCameraManager.getComponent(ID);

		// Check for change in projection parameters
		if (camera.fov != camera.lastFov || camera.aspect != camera.lastAspect || camera.zNear != camera.lastZNear || camera.zFar != camera.lastZFar) 
		{
			camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar); // Update projection matrix

			camera.lastFov = camera.fov;
			camera.lastAspect = camera.aspect;
			camera.lastZNear = camera.zNear;
			camera.lastZFar = camera.zFar;

			// Invoke projection changed callbacks
			for (unsigned int i = 0; i < camera.projectionChangedCallbacks.length; i++)
				(*camera.projectionChangedCallbacks[i])(camera);
		}
	}
}
