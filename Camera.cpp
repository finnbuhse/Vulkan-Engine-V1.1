#include "Camera.h"

unsigned int Camera::subscribeProjectionChangedEvent(const std::function<void(const Camera&)>& callback)
{
	projectionChangedCallbacks.push(callback);
	return projectionChangedCallbacks.length - 1;
}

void Camera::unsubscribeProjectionChangedEvent(const unsigned int& index)
{
	projectionChangedCallbacks.remove(index);
}

unsigned int Camera::subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>& callback)
{
	viewChangedCallbacks.push(callback);
	return viewChangedCallbacks.length - 1;
}

void Camera::unsubscribeViewChangedEvent(const unsigned int& index)
{
	viewChangedCallbacks.remove(index);
}

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

CameraSystem::CameraSystem()
{
	ComponentManager<Transform>& transformManager = ComponentManager<Transform>::instance();

	mComposition = transformManager.bit | mCameraManager.bit;

	transformManager.subscribeAddEvent(std::bind(&CameraSystem::componentAdded, this, std::placeholders::_1));
	transformManager.subscribeRemoveEvent(std::bind(&CameraSystem::componentRemoved, this, std::placeholders::_1));
	mCameraManager.subscribeAddEvent(std::bind(&CameraSystem::componentAdded, this, std::placeholders::_1));
	mCameraManager.subscribeRemoveEvent(std::bind(&CameraSystem::componentRemoved, this, std::placeholders::_1));
}

CameraSystem& CameraSystem::instance()
{
	static CameraSystem instance;
	return instance;
}

void CameraSystem::componentAdded(const Entity& entity)
{
	if ((entity.composition() & mComposition) == mComposition)
	{
		Camera& camera = entity.getComponent<Camera>();
		camera.projectionChangedCallbacks.initialize();
		camera.viewChangedCallbacks.initialize();
		camera.transformChangedCallbackIndex = entity.getComponent<Transform>().subscribeChangedEvent(std::bind(&CameraSystem::transformChanged, this, std::placeholders::_1));
		mEntityIDs.push_back(entity.ID());
	}
}

void CameraSystem::componentRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator IDIterator = std::find(mEntityIDs.begin(), mEntityIDs.end(), entity.ID());
	if (IDIterator != mEntityIDs.end())
	{
		Camera& camera = entity.getComponent<Camera>();
		camera.projectionChangedCallbacks.free();
		camera.viewChangedCallbacks.free();
		entity.getComponent<Transform>().unsubscribeChangedEvent(camera.transformChangedCallbackIndex);	
		mEntityIDs.erase(IDIterator);
	}
}

void CameraSystem::transformChanged(const Transform& transform) const
{
	Camera& camera = mCameraManager.getComponent(transform.entityID);
	camera.viewMatrix = glm::lookAt(transform.worldPosition, transform.worldPosition + transform.worldDirection(), glm::vec3(0.0f, 1.0f, 0.0f));

	for (unsigned int i = 0; i < camera.viewChangedCallbacks.length; i++)
		camera.viewChangedCallbacks[i](transform, camera);
}

void CameraSystem::update() const
{
	for (const EntityID& ID : mEntityIDs)
	{
		Camera& camera = mCameraManager.getComponent(ID);
		if (camera.fov != camera.lastFov || camera.aspect != camera.lastAspect || camera.zNear != camera.lastZNear || camera.zFar != camera.lastZFar)
		{
			camera.projectionMatrix = glm::perspective(glm::radians(camera.fov), camera.aspect, camera.zNear, camera.zFar);

			camera.lastFov = camera.fov;
			camera.lastAspect = camera.aspect;
			camera.lastZNear = camera.zNear;
			camera.lastZFar = camera.zFar;

			for (unsigned int i = 0; i < camera.projectionChangedCallbacks.length; i++)
				camera.projectionChangedCallbacks[i](camera);
		}
	}
}
