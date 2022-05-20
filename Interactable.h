#pragma once
#include "Entity.h"
#include "Physics.h"

typedef std::function<void(Entity&)> InteractCallback;

struct Interactable
{
	Vector<InteractCallback*> interactCallbacks;

	void subscribeInteractEvent(const InteractCallback* callback);
	void unsubscribeInteractEvent(const InteractCallback* callback);
};

template <>
std::vector<char> serialize(const Interactable& interactable);

template <>
void deserialize(const std::vector<char>& vecData, Interactable& interactable);

struct Interactor
{
	float interactDistance;
};

class InteractSystem
{
private:
	PhysicsSystem& mPhysicsSystem = PhysicsSystem::instance();
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Interactor>& mInteractorManager = ComponentManager<Interactor>::instance();
	ComponentManager<RigidBody>& mRigidBodyManager = ComponentManager<RigidBody>::instance();
	ComponentManager<Interactable>& mInteractableManager = ComponentManager<Interactable>::instance();
	WindowManager& mWindowManager = WindowManager::instance();

	const ComponentAddedCallback mComponentAddedCallback = std::bind(&InteractSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponentRemovedCallback = std::bind(&InteractSystem::componentRemoved, this, std::placeholders::_1);
	const ComponentAddedCallback mInteractableComponentAddedCallback = std::bind(&InteractSystem::interactableComponentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mInteractableComponentRemovedCallback = std::bind(&InteractSystem::interactableComponentRemoved, this, std::placeholders::_1);

	const KeyCallback mInteractCallback = std::bind(&InteractSystem::interact, this);

	Composition mInteractorComposition;
	Composition mInteractableComposition;
	EntityID mInteractor = NULL;

	InteractSystem();

public:
	static InteractSystem& instance();

	InteractSystem(const InteractSystem& copy) = delete;
	~InteractSystem();

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void interactableComponentAdded(const Entity& entity);
	void interactableComponentRemoved(const Entity& entity);

	void interact();

	EntityID getInteractor() const;
};