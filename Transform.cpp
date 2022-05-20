#include "Transform.h"
#include <iostream>

void print(const glm::vec2& vec2)
{
	std::cout << vec2.x << ", " << vec2.y << "\n";
}

void print(const glm::vec3& vec3)
{
	std::cout << vec3.x << ", " << vec3.y << ", " << vec3.z << "\n";
}

void print(const glm::quat& quat)
{
	std::cout << quat.x << ", " << quat.y << ", " << quat.z << ", " << quat.w << "\n";
}

void print(const glm::mat3& matrix)
{
	for (unsigned int i = 0; i < 3; i++)
	{
		for (unsigned int j = 0; j < 3; j++)
		{
			std::cout << matrix[j][i];
			if (i < 2 || j < 2)
				std::cout << ", ";
		}
		std::cout << "\n";
	}
}

void print(const glm::mat4& matrix)
{
	for (unsigned int i = 0; i < 4; i++)
	{
		for (unsigned int j = 0; j < 4; j++)
		{
			std::cout << matrix[j][i];
			if (i < 3 || j < 3)
				std::cout << ", ";
		}
		std::cout << "\n";
	}
}

glm::vec3 Transform::direction(const glm::vec3& direction) const
{
	return rotation * direction;
}

glm::vec3 Transform::worldDirection(const glm::vec3& direction) const
{
	return glm::normalize(worldRotation * direction);
}

void Transform::translate(const glm::vec3& translation)
{
	position += translation;
}

void Transform::rotate(const float& angle, const glm::vec3& axis)
{
	rotation = glm::angleAxis(angle, axis) * rotation;
}

void Transform::enlarge(const glm::vec3& factor)
{
	scale *= factor;
}

void Transform::addChild(const Entity& child)
{
	TransformSystem& transformSystem = TransformSystem::instance();
	transformSystem.mEntityIDs.erase(std::find(transformSystem.mEntityIDs.begin(), transformSystem.mEntityIDs.end(), child.ID()));

	child.getComponent<Transform>().parentID = entityID;
	childrenIDs.push(child.ID());
}

void Transform::removeChild(const Entity& child)
{
	child.getComponent<Transform>().parentID = NULL;

	TransformSystem::instance().mEntityIDs.push_back(child.ID());

	childrenIDs.remove(childrenIDs.find(child.ID()));
}

void Transform::subscribeChangedEvent(const TransformChangedCallback* callback)
{
	changedCallbacks.push((TransformChangedCallback*)callback);
}

void Transform::unsubscribeChangedEvent(const TransformChangedCallback* callback)
{
	changedCallbacks.remove(changedCallbacks.find((TransformChangedCallback*)callback));
}

TransformCreateInfo::operator Transform() const
{
	Transform transform = {};
	transform.matrix = glm::mat4(1.0f);
	transform.lastPosition = glm::vec3(0.0f);
	transform.lastRotation = glm::vec3(0.0f);
	transform.lastScale = glm::vec3(1.0f);
	transform.position = position;
	transform.rotation = rotation;
	transform.scale = scale;
	transform.dynamic = dynamic;
	transform.worldPosition = glm::vec3(0.0f);
	transform.worldRotation = glm::vec3(0.0f);
	transform.worldScale = glm::vec3(1.0f);
	return transform;
}

template<>
std::vector<char> serialize(const Transform& transform)
{
	TransformCreateInfo serializeInfo;
	serializeInfo.position = transform.position;
	serializeInfo.rotation = transform.rotation;
	serializeInfo.scale = transform.scale;

	return serialize(serializeInfo);
}

template<>
void deserialize(const std::vector<char>& vecData, Transform& write)
{
	TransformCreateInfo createInfo;
	deserialize(vecData, createInfo);
	write.position = createInfo.position;
	write.rotation = createInfo.rotation;
	write.scale = createInfo.scale;
}

glm::mat3 translateMatrix(const glm::vec2& translation)
{
	glm::mat3 matrix;
	matrix[0] = { 1, 0, 0 };
	matrix[1] = { 0, 1, 0 };
	matrix[2] = { translation.x, translation.y, 1 };
	return matrix;
}

/*
Construct a 2D rotation matrix.
\param angle: Angle in radians of the rotation.
*/
glm::mat3 rotateMatrix(const float& angle)
{
	glm::mat3 matrix;
	float cos = glm::cos(angle);
	float sin = glm::sin(angle);
	matrix[0] = { cos, sin, 0 };
	matrix[1] = { -sin, cos, 0 };
	matrix[2] = { 0, 0, 1.0f };
	return matrix;
}

glm::mat3 enlargeMatrix(const glm::vec2 enlargement)
{
	glm::mat3 matrix(1.0f);
	matrix[0] = { enlargement.x, 0, 0 };
	matrix[1] = { 0, enlargement.y, 0 };
	matrix[2] = { 0, 0, 1.0f };
	return matrix;
}

void Transform2D::translate(const glm::vec2& translation)
{
	position += translation;
}

void Transform2D::rotate(const float& angle)
{
	rotation += angle;
}

void Transform2D::enlarge(const glm::vec2& factor)
{
	scale *= factor;
}

void Transform2D::addChild(const Entity& child)
{
	TransformSystem& transformSystem = TransformSystem::instance();
	transformSystem.mEntity2DIDs.erase(std::find(transformSystem.mEntity2DIDs.begin(), transformSystem.mEntity2DIDs.end(), child.ID()));

	child.getComponent<Transform2D>().parentID = entityID;
	childrenIDs.push(child.ID());
}

void Transform2D::removeChild(const Entity& child)
{
	child.getComponent<Transform2D>().parentID = NULL;

	TransformSystem::instance().mEntity2DIDs.push_back(child.ID());

	childrenIDs.remove(childrenIDs.find(child.ID()));
}

void Transform2D::subscribeChangedEvent(const Transform2DChangedCallback* callback)
{
	changedCallbacks.push((Transform2DChangedCallback*)callback);
}

void Transform2D::unsubscribeChangedEvent(const Transform2DChangedCallback* callback)
{
	changedCallbacks.remove(changedCallbacks.find((Transform2DChangedCallback*)callback));
}

Transform2DCreateInfo::operator Transform2D() const
{
	Transform2D transform = {};
	transform.matrix = glm::mat3(1.0f);
	transform.lastPosition = glm::vec2(0.0f);
	transform.lastRotation = 0.0f;
	transform.lastScale = glm::vec2(1.0f);
	transform.position = position;
	transform.rotation = rotation;
	transform.scale = scale;
	transform.dynamic = dynamic;
	transform.worldPosition = glm::vec2(0.0f);
	transform.worldRotation = 0.0f;
	transform.worldScale = glm::vec2(1.0f);
	return transform;
}

TransformSystem::TransformSystem()
{
	mTransformManager.subscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mTransform2DManager.subscribeAddedEvent(&mComponent2DAddedCallback);
	mTransform2DManager.subscribeRemovedEvent(&mComponent2DRemovedCallback);
}

void TransformSystem::updateTransform(const EntityID& entityID) const
{
	Transform& transform = mTransformManager.getComponent(entityID);
	if (transform.dynamic)
	{
		// Check for change in position, rotation, and scale.
		transform.positionChanged = transform.position != transform.lastPosition;
		transform.rotationChanged = transform.rotation != transform.lastRotation;
		transform.scaleChanged = transform.scale != transform.lastScale;

		if (transform.parentID)
		{
			const Transform& parentTransform = mTransformManager.getComponent(transform.parentID);

			// World position is also changed if it's parents position, rotation, or scale has changed.
			transform.positionChanged |= parentTransform.positionChanged || parentTransform.rotationChanged || parentTransform.scaleChanged;

			// World rotation is also changed if it's parents rotation has changed.
			transform.rotationChanged |= parentTransform.rotationChanged;

			// Scale is also changed if it's parents scale has changed.
			transform.scaleChanged |= parentTransform.scaleChanged;

			if (transform.positionChanged || transform.rotationChanged || transform.scaleChanged)
			{
				// Transformation matrix must be updated if any change has occurred.
				transform.matrix = parentTransform.matrix * glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale);

				if (transform.positionChanged)
				{
					glm::vec4 temp = parentTransform.matrix * glm::vec4(transform.position, 1.0f);
					transform.worldPosition = glm::vec3(temp.x, temp.y, temp.z); // Update world position.
				}

				if (transform.rotationChanged)
					transform.worldRotation = parentTransform.worldRotation * transform.rotation; // Update world rotation.

				if (transform.scaleChanged)
					transform.worldScale = parentTransform.worldScale * transform.scale; // Update world scale.

				// Invoke changed callbacks.
				for (unsigned int i = 0; i < transform.changedCallbacks.length; i++)
					(*transform.changedCallbacks[i])(transform);

				transform.lastPosition = transform.position;
				transform.lastRotation = transform.rotation;
				transform.lastScale = transform.scale;
			}
		}
		else
		{
			if (transform.positionChanged || transform.rotationChanged || transform.scaleChanged)
			{
				transform.matrix = glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale);

				transform.worldPosition = transform.position;
				transform.worldRotation = transform.rotation;
				transform.worldScale = transform.scale;

				for (unsigned int i = 0; i < transform.changedCallbacks.length; i++)
					(*transform.changedCallbacks[i])(transform);

				transform.lastPosition = transform.position;
				transform.lastRotation = transform.rotation;
				transform.lastScale = transform.scale;
			}
		}

		// Update children.
		for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
			updateTransform(transform.childrenIDs[i]);
	}
}

void TransformSystem::updateTransform2D(const EntityID& entityID) const
{
	Transform2D& transform = mTransform2DManager.getComponent(entityID);

	if (transform.dynamic)
	{
		// Check for change in position, rotation, and scale.
		transform.positionChanged = transform.position != transform.lastPosition;
		transform.rotationChanged = transform.rotation != transform.lastRotation;
		transform.scaleChanged = transform.scale != transform.lastScale;

		if (transform.parentID)
		{
			const Transform2D& parentTransform = mTransform2DManager.getComponent(transform.parentID);

			transform.positionChanged |= parentTransform.positionChanged || parentTransform.rotationChanged || parentTransform.scaleChanged;
			transform.rotationChanged |= parentTransform.rotationChanged;
			transform.scaleChanged |= parentTransform.scaleChanged;

			if (transform.positionChanged || transform.rotationChanged || transform.scaleChanged)
			{
				transform.localMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(transform.position, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(transform.scale, 1.0f));
				transform.matrix = parentTransform.matrix * transform.localMatrix;

				if (transform.positionChanged)
				{
					glm::vec4 temp = parentTransform.matrix * glm::vec4(transform.position, 0.0f, 1.0f);
					transform.worldPosition = glm::vec2(temp.x, temp.y);
				}
				if (transform.rotationChanged)
					transform.worldRotation = parentTransform.worldRotation + transform.rotation;

				if (transform.scaleChanged)
					transform.worldScale = parentTransform.worldScale * transform.scale;

				for (unsigned int i = 0; i < transform.changedCallbacks.length; i++)
					(*transform.changedCallbacks[i])(transform);

				transform.lastPosition = transform.position;
				transform.lastRotation = transform.rotation;
				transform.lastScale = transform.scale;
			}
		}
		else
		{
			if (transform.positionChanged || transform.rotationChanged || transform.scaleChanged)
			{
				transform.matrix = transform.localMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(transform.position, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(transform.scale, 1.0f));

				transform.worldPosition = transform.position;
				transform.worldRotation = transform.rotation;
				transform.worldScale = transform.scale;

				for (unsigned int i = 0; i < transform.changedCallbacks.length; i++)
					(*transform.changedCallbacks[i])(transform);

				transform.lastPosition = transform.position;
				transform.lastRotation = transform.rotation;
				transform.lastScale = transform.scale;
			}
		}

		// Update children.
		for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
			updateTransform2D(transform.childrenIDs[i]);
	}
}

TransformSystem& TransformSystem::instance()
{
	static TransformSystem instance;
	return instance;
}

TransformSystem::~TransformSystem()
{
	mTransformManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);
	mTransform2DManager.unsubscribeAddedEvent(&mComponent2DAddedCallback);
	mTransform2DManager.unsubscribeRemovedEvent(&mComponent2DRemovedCallback);
}

void TransformSystem::componentAdded(const Entity& entity)
{
	Transform& transform = entity.getComponent<Transform>();
	transform.childrenIDs.initialize();
	transform.changedCallbacks.initialize();
	transform.entityID = entity.ID();
	transform.parentID = NULL;

	mEntityIDs.push_back(entity.ID());
}

void TransformSystem::componentRemoved(const Entity& entity)
{
	Transform& transform = entity.getComponent<Transform>();
	if (transform.parentID)
		mTransformManager.getComponent(transform.parentID).removeChild(entity);

	for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
	{
		mTransformManager.getComponent(transform.childrenIDs[i]).parentID = NULL;
		mEntityIDs.push_back(transform.childrenIDs[i]);
	}
	transform.childrenIDs.free();
	transform.changedCallbacks.free();

	mEntityIDs.erase(std::find(mEntityIDs.begin(), mEntityIDs.end(), entity.ID()));
}

void TransformSystem::component2DAdded(const Entity& entity)
{
	Transform2D& transform = entity.getComponent<Transform2D>();
	transform.childrenIDs.initialize();
	transform.changedCallbacks.initialize();
	transform.entityID = entity.ID();
	transform.parentID = NULL;

	mEntity2DIDs.push_back(entity.ID());
}

void TransformSystem::component2DRemoved(const Entity& entity)
{
	Transform2D& transform = entity.getComponent<Transform2D>();
	if (transform.parentID)
		mTransform2DManager.getComponent(transform.parentID).removeChild(entity);

	for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
	{
		mTransform2DManager.getComponent(transform.childrenIDs[i]).parentID = NULL;
		mEntity2DIDs.push_back(transform.childrenIDs[i]);
	}
	transform.childrenIDs.free();
	transform.changedCallbacks.free();

	mEntity2DIDs.erase(std::find(mEntity2DIDs.begin(), mEntity2DIDs.end(), entity.ID()));
}

void TransformSystem::update() const
{
	for (const EntityID& ID : mEntityIDs)
		updateTransform(ID);

	for (const EntityID& ID : mEntity2DIDs)
		updateTransform2D(ID);
}
