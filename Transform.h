#pragma once
#include "ComponentManager.h"
#include "Vector.h"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

void print(const glm::vec2& vec2);
void print(const glm::vec3& vec3);
void print(const glm::quat& quat);
void print(const glm::mat3& matrix);
void print(const glm::mat4& matrix);

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

glm::mat3 translateMatrix(const glm::vec2& translation);

/*
Construct a 2D rotation matrix.
\param angle: Angle in radians of the rotation.
*/
glm::mat3 rotateMatrix(const float& angle);

glm::mat3 enlargeMatrix(const glm::vec2 enlargement);

struct Transform2D;
typedef std::function<void(Transform2D&)> Transform2DChangedCallback;

struct Transform2D
{
	Vector<EntityID> childrenIDs;

	Vector<Transform2DChangedCallback*> changedCallbacks;

	EntityID entityID;
	EntityID parentID;

	glm::mat3 matrix;

	glm::vec2 lastPosition;
	float lastRotation;
	glm::vec2 lastScale;

	glm::vec2 position;
	float rotation;
	glm::vec2 scale;

	glm::vec2 worldPosition;
	float worldRotation;
	glm::vec2 worldScale;

	bool positionChanged;
	bool rotationChanged;
	bool scaleChanged;

	void translate(const glm::vec2& translation);
	void rotate(const float& angle);
	void enlarge(const glm::vec2& factor);

	void addChild(const Entity& child);
	void removeChild(const Entity& child);

	void subscribeChangedEvent(const Transform2DChangedCallback* callback);
	void unsubscribeChangedEvent(const Transform2DChangedCallback* callback);
};

struct Transform2DCreateInfo
{
	glm::vec2 position = glm::vec2(0.0f);
	float rotation = 0.0f;
	glm::vec2 scale = glm::vec2(1.0f);

	operator Transform2D() const;
};

template<>
std::vector<char> serialize(const Transform& transform);

template<>
void deserialize(const std::vector<char>& vecData, Transform& write);

class TransformSystem
{
private:
	friend Transform;
	friend Transform2D;

	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Transform2D>& mTransform2DManager = ComponentManager<Transform2D>::instance();
	const ComponentAddedCallback mComponentAddedCallback = std::bind(&TransformSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponentRemovedCallback = std::bind(&TransformSystem::componentRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mComponent2DAddedCallback = std::bind(&TransformSystem::component2DAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponent2DRemovedCallback = std::bind(&TransformSystem::component2DRemoved, this, std::placeholders::_1);
	
	// All transforms that have no parent, but may have children.
	std::vector<EntityID> mEntityIDs;
	std::vector<EntityID> mEntity2DIDs;

	TransformSystem();

	void updateTransform(const EntityID& entityID) const;
	void updateTransform2D(const EntityID& entityID) const;

public:
	static TransformSystem& instance();

	~TransformSystem();
	TransformSystem(const TransformSystem& copy) = delete;

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void component2DAdded(const Entity& entity);
	void component2DRemoved(const Entity & entity);

	void update() const;
};
