#pragma once
#include "Transform.h"

struct Camera;

struct Camera
{
	// Dynamic array containing references to callbacks to be invoked when the camera's projection changes e.g FOV, aspect ratio, etc
	Vector<std::function<void(const Camera&)>*> projectionChangedCallbacks;
	// Dynamic array containing references to callbacks to be invoked when the camera's view changes e.g position, rotation.
	Vector<std::function<void(const Transform&, const Camera&)>*> viewChangedCallbacks;

	// Matrix which performs perspective projection
	glm::mat4 projectionMatrix;
	// Matrix which positions the scene for correct view
	glm::mat4 viewMatrix;

	// Store last frames state; enables camera system to detect changes and update matrices
	float lastFov;
	float lastAspect;
	float lastZNear, lastZFar;

	// Can be freely assigned to in order to adjust certain camera characteristics
	float fov;
	float aspect;
	float zNear, zFar;

	/*
	Add a procedure to get automatically invoked whenever the camera's projection changes.
	\param callback: Pointer to the function to be added.
	*/
	void subscribeProjectionChangedEvent(const std::function<void(const Camera&)>* callback);

	/*
	Remove a procedure from the array of procedures that get invoked once the camera's projection changes.
	\param callback: Pointer to the function to be removed.
	*/
	void unsubscribeProjectionChangedEvent(const std::function<void(const Camera&)>* callback);

	/*
	Add a procedure to get automatically invoked whenever the camera's view (position or rotation) changes.
	\param callback: Pointer to the function to be added.
	*/
	void subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>* callback);

	/*
	Remove a procedure from the array of procedures that get invoked once the camera's view changes.
	\param callback: Pointer to the function to be removed.
	*/
	void unsubscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>* callback);
};

struct CameraCreateInfo
{
	float fov;
	float aspect;
	float zNear, zFar;

	// 'Constructs' Camera 
	operator Camera() const;
};

template<>
std::vector<char> serialize(const Camera& camera);

template<>
void deserialize(const std::vector<char>& vecData, Camera& write);

class CameraSystem
{
private:
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();

	const ComponentAddedCallback mComponentAddedCallback = std::bind(&CameraSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponentRemovedCallback = std::bind(&CameraSystem::componentRemoved, this, std::placeholders::_1);

	const TransformChangedCallback mTransformChangedCallback = std::bind(&CameraSystem::transformChanged, this, std::placeholders::_1);

	Composition mComposition;
	std::vector<EntityID> mEntityIDs;

	CameraSystem();

public:
	static CameraSystem& instance();

	CameraSystem(const CameraSystem& copy) = delete;
	~CameraSystem();

	// Invoked after a component of interest to the CameraSystem is added to an entity. Should not be used outside of it's internal use.
	void componentAdded(const Entity& entity);

	// Invoked before a component of interest to the CameraSystem is removed from an entity. Should not be used outside of it's internal use.
	void componentRemoved(const Entity& entity);

	// Invoked when a camera entity's transform has changed. Should not be used outside of it's internal use
	void transformChanged(Transform& transform) const;

	// Invoked every frame. Should not be used outside of it's internal use.
	void update() const;
};