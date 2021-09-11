#pragma once
#include "ComponentManager.h"
#include "Vector.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

struct Transform
{
	Vector<EntityID> childrenIDs;
	Vector<std::function<void(const Transform&)>> changedCallbacks;

	EntityID entityID;
	EntityID parentID;

	glm::mat4 matrix;

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

	/* Contains only parent transforms, once updated each transform updates its children this ensures
	   that when a transform is updated its parent is already up to date */
	std::vector<EntityID> mEntityIDs;

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