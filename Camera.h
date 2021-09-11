#pragma once
#include "Transform.h"

struct Camera
{
	Vector<std::function<void(const Camera&)>> projectionChangedCallbacks;
	Vector<std::function<void(const Transform&, const Camera&)>> viewChangedCallbacks;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	float lastFov;
	float lastAspect;
	float lastZNear, lastZFar;

	float fov;
	float aspect;
	float zNear, zFar;

	unsigned int transformChangedCallbackIndex;

	unsigned int subscribeProjectionChangedEvent(const std::function<void(const Camera&)>& callback);
	void unsubscribeProjectionChangedEvent(const unsigned int& index);
	unsigned int subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>& callback);
	void unsubscribeViewChangedEvent(const unsigned int& index);
};

struct CameraCreateInfo
{
	float fov;
	float aspect;
	float zNear, zFar;

	operator Camera() const;
};

class CameraSystem
{
private:
	Composition mComposition;
	std::vector<EntityID> mEntityIDs;

	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();

	CameraSystem();

public:
	static CameraSystem& instance();

	CameraSystem(const CameraSystem& copy) = delete;

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);
	void transformChanged(const Transform& transform) const;
	void update() const;
};