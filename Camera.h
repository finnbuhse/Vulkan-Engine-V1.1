#pragma once
#include "Transform.h"

struct Camera;

struct CameraCreateInfo
{
	float fov;
	float aspect;
	float zNear, zFar;

	operator Camera() const;
};

struct Camera
{
	Vector<std::function<void(const Camera&)>*> projectionChangedCallbacks;
	Vector<std::function<void(const Transform&, const Camera&)>*> viewChangedCallbacks;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	float lastFov;
	float lastAspect;
	float lastZNear, lastZFar;

	float fov;
	float aspect;
	float zNear, zFar;

	/*
	Add a procedure to get automatically invoked whenever the camera's projection changes.
	\param callback: Pointer to the procedure to be added. Procedure must follow template: void [procedure name](const Camera& [camera name]).
	*/
	void subscribeProjectionChangedEvent(const std::function<void(const Camera&)>* callback);

	/*
	Remove a procedure from the array of procedures that get invoked once the camera's projection changes.
	\param callback: Pointer to the procedure to be removed. Procedure must follow template: void [procedure name](const Camera& [camera name]).
	*/
	void unsubscribeProjectionChangedEvent(const std::function<void(const Camera&)>* callback);

	/*
	Add a procedure to get automatically invoked whenever the camera's view (position or rotation) changes.
	\param callback: Pointer to the procedure to be added. Procedure must follow template: void [procedure name](const Transform& [transform name], const Camera& [camera name]).
	*/
	void subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>* callback);

	/*
	Remove a procedure from the array of procedures that get invoked once the camera's view changes.
	\param callback: Pointer to the procedure to be removed. Procedure must follow template: void [[procedure name](const Transform& [transform name], const Camera& [camera name]).
	*/
	void unsubscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>* callback);
};

template<>
std::vector<char> serialize(const Camera& camera);

template<>
void deserialize(const std::vector<char>& vecData, Camera& write);

class CameraSystem
{
private:
	const TransformChangedCallback transformChangedCallback = std::bind(&CameraSystem::transformChanged, this, std::placeholders::_1);

	Composition mComposition;
	std::vector<EntityID> mEntityIDs;

	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();

	CameraSystem();

public:
	static CameraSystem& instance();

	CameraSystem(const CameraSystem& copy) = delete;

	/*
	Invoked after a component of interest to the CameraSystem is added to an entity. Should not be used outside of it's internal use.
	*/
	void componentAdded(const Entity& entity);

	/*
	Invoked before a component of interest to the CameraSystem is removed from an entity. Should not be used outside of it's internal use.
	*/
	void componentRemoved(const Entity& entity);

	/*
	Invoked when a 'Camera' entity's transform has changed. Should not be used outside of it's internal use.
	*/
	void transformChanged(Transform& transform) const;

	/*
	Invoked every frame. Should not be used outside of it's internal use.
	*/
	void update() const;
};