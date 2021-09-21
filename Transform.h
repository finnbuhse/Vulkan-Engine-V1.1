#pragma once
#include "ComponentManager.h"
#include "Vector.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Transform
{   /* Dynamic array storing the entity IDs of the transform's children.
	   IDs should NOT be directly appended and removed from the array but one should use addChild and removeChild */
	Vector<EntityID> childrenIDs; 

	/* Dynamic array of callbacks to invoke upon change.
	   Callbacks should NOT be directly appended and removed from the array but one should use subscribeChangedEvent and unsubscribeChangedEvent */
	Vector<std::function<void(const Transform&)>> changedCallbacks;

	EntityID entityID; // The transform's entity's ID
	EntityID parentID; // The transform's parent's ID

	glm::mat4 matrix; // Matrix representing the translation, rotation, and enlargment to transform the object from the origin with no rotation and scale to its final state in world space

	// Detect change by comparing with last frame
	glm::vec3 lastPosition;
	glm::quat lastRotation;
	glm::vec3 lastScale;

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	glm::vec3 worldPosition;
	glm::quat worldRotation;
	glm::vec3 worldScale;

	// Flags set before changed event invoking changed callbacks
	bool positionChanged;
	bool rotationChanged;
	bool scaleChanged;

	glm::vec3 direction(const glm::vec3& direction = glm::vec3(0.0f, 0.0f, -1.0f)) const;
	glm::vec3 worldDirection(const glm::vec3& direction = glm::vec3(0.0f, 0.0f, -1.0f)) const;

	void translate(const glm::vec3& translation);
	void rotate(const float& angle, const glm::vec3& axis);
	void enlarge(const glm::vec3& factor);

	// Interface with childrenIDs
	void addChild(const Entity& child);
	void removeChild(const Entity& child);

	// Interface with changedCallbacks
	unsigned int subscribeChangedEvent(const std::function<void(const Transform&)>& callback);
	void unsubscribeChangedEvent(const unsigned int& index);
};

struct TransformCreateInfo
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	operator Transform() const;
};

class TransformSystem
{
private:
	friend Transform;
	
	std::vector<EntityID> mEntityIDs; // Includes all transforms that have no parent, but may have children

	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();

	TransformSystem();

	void updateTransform(const EntityID& entityID) const;

public:
	static TransformSystem& instance();

	TransformSystem(const TransformSystem& copy) = delete;

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void update() const;
};