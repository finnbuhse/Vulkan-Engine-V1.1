#include "Interactable.h"
#include <iostream>

void Interactable::subscribeInteractEvent(const InteractCallback* callback)
{
	interactCallbacks.push((InteractCallback*)callback);
}

void Interactable::unsubscribeInteractEvent(const InteractCallback* callback)
{
	interactCallbacks.remove(interactCallbacks.find((InteractCallback*)callback));
}

template <>
std::vector<char> serialize(const Interactable& interactable)
{
	return {};
}

template <>
void deserialize(const std::vector<char>& vecData, Interactable& interactable)
{

}

InteractSystem::InteractSystem()
{
	mTransformManager.subscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mInteractorManager.subscribeAddedEvent(&mComponentAddedCallback);
	mInteractorManager.subscribeRemovedEvent(&mComponentRemovedCallback);

	mTransformManager.subscribeAddedEvent(&mInteractableComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mInteractableComponentRemovedCallback);
	mRigidBodyManager.subscribeAddedEvent(&mInteractableComponentAddedCallback);
	mRigidBodyManager.subscribeRemovedEvent(&mInteractableComponentRemovedCallback);
	mInteractableManager.subscribeAddedEvent(&mInteractableComponentAddedCallback);
	mInteractableManager.subscribeRemovedEvent(&mInteractableComponentRemovedCallback);

	mWindowManager.subscribeKeyPressEvent(E, &mInteractCallback);

	mInteractorComposition = mTransformManager.bit | mInteractorManager.bit;
	mInteractableComposition = mTransformManager.bit | mRigidBodyManager.bit | mInteractableManager.bit;
}

InteractSystem& InteractSystem::instance()
{
	static InteractSystem instance;
	return instance;
}

InteractSystem::~InteractSystem()
{
	mTransformManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);
	mInteractorManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mInteractorManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);

	mTransformManager.unsubscribeAddedEvent(&mInteractableComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mInteractableComponentRemovedCallback);
	mRigidBodyManager.unsubscribeAddedEvent(&mInteractableComponentAddedCallback);
	mRigidBodyManager.unsubscribeRemovedEvent(&mInteractableComponentRemovedCallback);
	mInteractableManager.unsubscribeAddedEvent(&mInteractableComponentAddedCallback);
	mInteractableManager.unsubscribeRemovedEvent(&mInteractableComponentRemovedCallback);

	mWindowManager.unsubscribeKeyPressEvent(E, &mInteractCallback);
}

void InteractSystem::componentAdded(const Entity& entity)
{
	if ((entity.composition() & mInteractorComposition) == mInteractorComposition)
		mInteractor = entity.ID();
}

void InteractSystem::componentRemoved(const Entity& entity)
{
	if (entity.ID() == mInteractor)
		mInteractor = NULL;
}

void InteractSystem::interactableComponentAdded(const Entity& entity)
{
	if ((entity.composition() & mInteractableComposition) == mInteractableComposition)
	{
		entity.getComponent<Interactable>().interactCallbacks.initialize();
	}
}

void InteractSystem::interactableComponentRemoved(const Entity& entity)
{
	if ((entity.composition() & mInteractableComposition) == mInteractableComposition)
	{
		entity.getComponent<Interactable>().interactCallbacks.free();
	}
}

void InteractSystem::interact()
{
	if (mInteractor)
	{
		Transform& transform = mTransformManager.getComponent(mInteractor);
		Interactor& interactor = mInteractorManager.getComponent(mInteractor);

		physx::PxRaycastBuffer hit = mPhysicsSystem.raycast(transform.worldPosition, transform.worldDirection(), interactor.interactDistance);

		if (hit.hasBlock)
		{
			Entity hitEntity(*((unsigned int*)hit.block.actor->userData));

			if (hitEntity.hasComponent<Interactable>())
			{
				Interactable& interactable = hitEntity.getComponent<Interactable>();
				for (unsigned int i = 0; i < interactable.interactCallbacks.length; i++)
					(*interactable.interactCallbacks[i])(hitEntity);
			}
		}
	}
}

EntityID InteractSystem::getInteractor() const
{
	return mInteractor;
}
