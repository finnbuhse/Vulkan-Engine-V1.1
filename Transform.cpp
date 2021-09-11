#include "Transform.h"

glm::vec3 Transform::direction(const glm::vec3& direction) const
{
	return rotation * direction;
}

glm::vec3 Transform::worldDirection(const glm::vec3& direction) const
{
	return worldRotation * direction;
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
	child.getComponent<Transform>().parentID = entityID;

	TransformSystem& transformSystem = TransformSystem::instance();
	transformSystem.mEntityIDs.erase(std::find(transformSystem.mEntityIDs.begin(), transformSystem.mEntityIDs.end(), child.ID())); // Assuming the child had no parent previous to this procedure (it shouldn't have); it would be in the system, and must be removed
	
	childrenIDs.push(child.ID());
}

void Transform::removeChild(const Entity& child)
{
	child.getComponent<Transform>().parentID = NULL; // Set child's parent to NULL

	TransformSystem::instance().mEntityIDs.push_back(child.ID()); // The child won't be in the system and must be added

	childrenIDs.remove(childrenIDs.find(child.ID()));
}

unsigned int Transform::subscribeChangedEvent(const std::function<void(const Transform&)>& callback)
{
	changedCallbacks.push(callback);
	return changedCallbacks.length - 1;
}

void Transform::unsubscribeChangedEvent(const unsigned int& index)
{
	changedCallbacks.remove(index);
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
	transform.worldPosition = glm::vec3(0.0f);
	transform.worldRotation = glm::vec3(0.0f);
	transform.worldScale = glm::vec3(1.0f);
	return transform;
}

TransformSystem::TransformSystem()
{
	mTransformManager.subscribeAddEvent(std::bind(&TransformSystem::componentAdded, this, std::placeholders::_1));
	mTransformManager.subscribeRemoveEvent(std::bind(&TransformSystem::componentRemoved, this, std::placeholders::_1));
}

void TransformSystem::updateTransform(const EntityID& entityID) const
{
	Transform& transform = mTransformManager.getComponent(entityID);

	transform.positionChanged = transform.position != transform.lastPosition;
	transform.rotationChanged = transform.rotation != transform.lastRotation;
	transform.scaleChanged = transform.scale != transform.lastScale;
	if (transform.parentID)
	{
		const Transform& parentTransform = mTransformManager.getComponent(transform.parentID);

		transform.positionChanged |= parentTransform.positionChanged || parentTransform.rotationChanged || parentTransform.scaleChanged;
		transform.rotationChanged |= parentTransform.rotationChanged;
		transform.scaleChanged |= parentTransform.scaleChanged;
		if (transform.positionChanged || transform.rotationChanged || transform.scaleChanged)
		{
			transform.matrix = parentTransform.matrix * glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale);

			if (transform.positionChanged)
			{
				transform.lastPosition = transform.position;
				transform.worldPosition = parentTransform.worldPosition + transform.position;
			}
			if (transform.rotationChanged)
			{
				transform.lastRotation = transform.rotation;
				transform.worldRotation = parentTransform.worldRotation * transform.rotation;
			}
			if (transform.scaleChanged)
			{
				transform.lastScale = transform.scale;
				transform.worldScale = parentTransform.worldScale * transform.scale;
			}

			for (unsigned int i = 0; i < transform.changedCallbacks.length; i++)
				transform.changedCallbacks[i](transform);
		}
	}
	else
	{
		if (transform.positionChanged || transform.rotationChanged || transform.scaleChanged)
		{
			transform.matrix = glm::translate(glm::mat4(1.0f), transform.position) * glm::mat4_cast(transform.rotation) * glm::scale(glm::mat4(1.0f), transform.scale);

			transform.lastPosition = transform.position;
			transform.worldPosition = transform.position;
			transform.lastRotation = transform.rotation;
			transform.worldRotation = transform.rotation;
			transform.lastScale = transform.scale;
			transform.worldScale = transform.scale;

			for (unsigned int i = 0; i < transform.changedCallbacks.length; i++)
				transform.changedCallbacks[i](transform);
		}
	}

	for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
		updateTransform(transform.childrenIDs[i]);
}

TransformSystem& TransformSystem::instance()
{
	static TransformSystem instance;
	return instance;
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

void TransformSystem::update() const
{
	for (const EntityID& ID : mEntityIDs)
		updateTransform(ID);
}
