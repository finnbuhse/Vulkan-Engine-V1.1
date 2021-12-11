#pragma once
#include "ComponentManager.h"
#include "Vector.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

void printVec3(const glm::vec3& vec3);
void printMat4(const glm::mat4& matrix);

struct Transform;

typedef std::function<void(Transform&)> TransformChangedCallback;

struct Transform
{   
	/* Stores the IDs of the transform's children.
	   IDs should NOT be directly appended and removed from the array but one should use the addChild and removeChild methods. */
	Vector<EntityID> childrenIDs; 

	/* Stores callbacks to invoke upon change.
	   Callbacks should NOT be directly appended and removed from the array but one should use subscribeChangedEvent and unsubscribeChangedEvent methods. */
	Vector<TransformChangedCallback*> changedCallbacks;

	EntityID entityID;
	EntityID parentID;

	glm::mat4 matrix; // Matrix representing a translation, rotation, and enlargment to transform the object from the origin with no rotation and identity scale to its final state in world space.

	// Detect change by comparing with last frame.
	glm::vec3 lastPosition;
	glm::quat lastRotation;
	glm::vec3 lastScale;

	glm::vec3 position;
	glm::quat rotation;
	glm::vec3 scale;

	glm::vec3 worldPosition;
	glm::quat worldRotation;
	glm::vec3 worldScale;

	bool positionChanged;
	bool rotationChanged;
	bool scaleChanged;

	glm::vec3 direction(const glm::vec3& direction = glm::vec3(0.0f, 0.0f, -1.0f)) const;
	glm::vec3 worldDirection(const glm::vec3& direction = glm::vec3(0.0f, 0.0f, -1.0f)) const;

	void translate(const glm::vec3& translation);
	void rotate(const float& angle, const glm::vec3& axis);
	void enlarge(const glm::vec3& factor);

	void addChild(const Entity& child);
	void removeChild(const Entity& child);

	void subscribeChangedEvent(const TransformChangedCallback* callback);
	void unsubscribeChangedEvent(const TransformChangedCallback* callback);
};

struct TransformCreateInfo
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);

	operator Transform() const;
};

template<>
std::vector<char> serialize(const Transform& transform);

template<>
void deserialize(const std::vector<char>& vecData, Transform& write);

class TransformSystem
{
private:
	friend Transform;

	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	const ComponentAddedCallback mComponentAddedCallback = std::bind(&TransformSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponentRemovedCallback = std::bind(&TransformSystem::componentRemoved, this, std::placeholders::_1);
	
	std::vector<EntityID> mEntityIDs; // Includes all transforms that have no parent, but may have children.

	TransformSystem();

	void updateTransform(const EntityID& entityID) const;

public:
	static TransformSystem& instance();

	~TransformSystem();
	TransformSystem(const TransformSystem& copy) = delete;

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void update() const;
};
