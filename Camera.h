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
	// Callbacks to be invoked when the camera's projection or view changes
	Vector<std::function<void(const Camera&)>> projectionChangedCallbacks;
	Vector<std::function<void(const Transform&, const Camera&)>> viewChangedCallbacks;

	glm::mat4 projectionMatrix;
	glm::mat4 viewMatrix;

	// Detect change via comparison with last frame
	float lastFov;
	float lastAspect;
	float lastZNear, lastZFar;

	// Can be assigned at runtime; the system will detect the change and take care of the adjustments
	float fov;
	float aspect;
	float zNear, zFar;

	/* A callback is appended to the transforms dynamic array of callbacks so the camera can respond when it's transform changes,
	   the index of the callback in the array must be stored so it can be removed */
	unsigned int transformChangedCallbackIndex;

	// Interface with callback arrays
	unsigned int subscribeProjectionChangedEvent(const std::function<void(const Camera&)>& callback);
	void unsubscribeProjectionChangedEvent(const unsigned int& index);
	unsigned int subscribeViewChangedEvent(const std::function<void(const Transform&, const Camera&)>& callback);
	void unsubscribeViewChangedEvent(const unsigned int& index);
};

template<>
std::vector<char> serialize(const Camera& camera);

template<>
void deserialize(const std::vector<char>& vecData, Camera& write);

class CameraSystem
{
private:
	Composition mComposition; // Describes which components an entity must have to be included in the system
	std::vector<EntityID> mEntityIDs; // The set of entities in the system

	ComponentManager<Camera>& mCameraManager = ComponentManager<Camera>::instance();

	CameraSystem();

public:
	// Only one instance of each system should exist
	static CameraSystem& instance();
	CameraSystem(const CameraSystem& copy) = delete;

	/* Invoked after a component of interest to the system is added to an entity,
	   does not neccessarily mean the entity has the full required set of components,
	   so a check for this is performed and if the entity meets the criteria it is added to the system */
	void componentAdded(const Entity& entity);

	/* Invoked before a component of interest to the system is removed from an entity, 
	   which will cause the entity to be removed from the system since it no longer meets the criteria */
	void componentRemoved(const Entity& entity);

	// Invoked when a transform whos entity also has a camera experiences a change
	void transformChanged(Transform& transform) const;

	// Invoked every frame
	void update() const;
};