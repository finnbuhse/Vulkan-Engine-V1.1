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

/* To minimise cache misses, components must be stored contiguously meaning they have to be Plain Old Data (POD).
   For an object to be POD it cannot have any constructors or deconstructors, use CreateInfos which cast to component types, normally just sets member variables.
   Majority of 'construction' such as subscribing to events and initializing Vectors (See Vector.h) happens in the componentAdded subroutines
   since components can equally be 'deconstructed' in the componentRemoved subroutines, this complements the fact that components are redundant
   unless they are included in their respective system, and so their data is initialized when added to the system, and destroyed when removed from the system. */
struct Camera
{
	// Dynamic arrays of callbacks mainly so the render system can respond to camera changes
	Vector<std::function<void(const Camera&)>> projectionChangedCallbacks;
	Vector<std::function<void(const Transform&, const Camera&)>> viewChangedCallbacks;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	// Detect change by comparing with last frame
	float lastFov;
	float lastAspect;
	float lastZNear, lastZFar;

	// These variables can simply be assigned at runtime; the system will detect the change and take care of the adjustments
	float fov;
	float aspect;
	float zNear, zFar;

	/* A callback is appended to the transforms dynamic array of callbacks so the camera can respond to its transforms change,
	   the index of the callback in the array must be stored so it can be removed */
	unsigned int transformChangedCallbackIndex;

	// Intended interface with callback arrays
	unsigned int subscribeProjectionChangedEvent(const std::function<void(const Camera&)>& callback);
	void unsubscribeProjectionChangedEvent(const unsigned int& index);
	unsigned int subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>& callback);
	void unsubscribeViewChangedEvent(const unsigned int& index);

	CameraCreateInfo serializeInfo() const;
};

// Systems operate on entities with a specific set of components
class CameraSystem
{
private:
	Composition mComposition; // A set of flags describing which components an entity must have to be added to the system
	std::vector<EntityID> mEntityIDs; // The set of entities in the system

	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();

	CameraSystem();

public:
	// Only one instance of each system should exist
	static CameraSystem& instance();
	CameraSystem(const CameraSystem& copy) = delete;

	/* Invoked everytime a component of interest to the system is added to an entity,
	   but does not neccessarily mean the entity has the full required set of components,
	   so a check for this is performed and if the entity meets the criteria it is added to the system */
	void componentAdded(const Entity& entity);

	/* Invoked before a component of interest to the system is removed from an entity, 
	   which will cause the entity to be removed from the system since it no longer meets the criteria */
	void componentRemoved(const Entity& entity);

	// Invoked everytime a transform whos entity also has a camera has changed
	void transformChanged(Transform& transform) const;

	// Invoked every frame
	void update() const;
};