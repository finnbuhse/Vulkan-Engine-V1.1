#pragma once
#include "Entity.h"
#include <functional>

typedef unsigned long long ComponentBit;

class ComponentManagerBase
{
private:
	static ComponentID queuedID;

	static std::unordered_map<ComponentID, ComponentManagerBase*> IDInstanceMap;

protected:
	ComponentManagerBase();

public:
	static ComponentManagerBase& componentManagerFromID(const ComponentID& ID);

	const ComponentID ID; // The unique ID identifying the managers component type.
	const ComponentBit bit; // The position of the bit in a Composition bitmask indicating possession of the component type.
	
	~ComponentManagerBase();
	
	/*
	Removes component from an entity. If invoked through pointer or reference they must point to a ComponentManager of some type T.
	\param The entity to remove the component from.
	*/
	virtual void removeComponent(const Entity& entity) = 0;

	virtual std::vector<char> getSerializedComponent(const EntityID& ID) = 0;
	virtual void addSerializedComponent(const std::vector<char>& vecData, Entity& entity) = 0;
};


template <typename T>
class ComponentManager : public ComponentManagerBase
{
private:
	std::unordered_map<EntityID, unsigned int> mEntityIndexMap;
	std::vector<T> mComponents;

	std::vector<std::function<void(const Entity&)>> mComponentAddedCallbacks;
	std::vector<std::function<void(const Entity&)>> mComponentRemovedCallbacks;

	ComponentManager() {};

public:
	static ComponentManager& instance() 
	{
		static ComponentManager instance;
		return instance;
	}

	ComponentManager(const ComponentManager& copy) = delete;

	/*
	Adds a component of type T to the entity.
	\param entity: The entity to add the component to.
	\param component: The component to add.
	\return A reference to the newly added component.
	*/
	T& addComponent(const Entity& entity, T component)
	{
		assert(("[ERROR] Cannot add component to entity that already possesses a component of that type", mEntityIndexMap.find(entity.ID()) == mEntityIndexMap.end()));
		mEntityIndexMap.insert({ entity.ID(), mComponents.size() }); // Entity ID now relates to the next free index for the component.
		mComponents.push_back(component); // Add component at index.

		entity.composition() |= bit; // Add component's bit to entity's composition.

		// Notify subscribers of 'component added' event.
		for (const std::function<void(const Entity&)>& callback : mComponentAddedCallbacks)
			callback(entity);

		return mComponents.back();
	}

	T& getComponent(const EntityID& entityID)
	{
		#ifdef NDEBUG
		return mComponents[mEntityIndexMap.at(entityID)];
		#else
		std::unordered_map<EntityID, unsigned int>::iterator IDIterator = mEntityIndexMap.find(entityID);
		assert(("[ERROR] Attempting to get component from an entity that does not possess a component of that type", IDIterator != mEntityIndexMap.end()));
		return mComponents[IDIterator->second];
		#endif
	}

	void removeComponent(const Entity& entity) override
	{
		// Find entity in entity index map.
		std::unordered_map<EntityID, unsigned int>::iterator IDIterator = mEntityIndexMap.find(entity.ID());
		assert(("[ERROR] Cannot remove component from an entity that does not possess a component of that type", IDIterator != mEntityIndexMap.end()));

		// Notify subscribers of 'component removed' event.
		for (const std::function<void(const Entity&)>& callback : mComponentRemovedCallbacks)
			callback(entity);

		entity.composition() &= ~bit; // Remove component's bit from entity's composition.
		
		// Find relation where it's index accesses the last element in 'mComponents', and assign the index to be the index of the component being removed.
		for (std::unordered_map<EntityID, unsigned int>::iterator it = mEntityIndexMap.begin(); it != mEntityIndexMap.end(); it++)
		{
			if (it->second == mComponents.size() - 1)
			{
				it->second = IDIterator->second;
				break;
			}
		}
		mComponents[IDIterator->second] = mComponents.back(); // Overwrite component being removed with last component. This ensures no gaps.

		mComponents.pop_back(); // Free last component.
		mEntityIndexMap.erase(IDIterator); // Remove entity's ID mapping.
	}

	std::vector<char> getSerializedComponent(const EntityID& ID) override
	{
		T& component = getComponent(ID);
		return serialize(component);
	}

	void addSerializedComponent(const std::vector<char>& vecData, Entity& entity) override
	{
		T& component = addComponent(entity, {});
		deserialize(vecData, component);
	}

	/*
	Add a procedure to get automatically invoked whenever a component of type T is added to an entity.
	\param callback: Pointer to the procedure to be added. Procedure must follow template: void [procedure name](const Entity& [entity name]).
	*/
	std::vector<std::function<void(const Entity& entity)>>::iterator subscribeAddEvent(const std::function<void(const Entity&)>& callback)
	{
		mComponentAddedCallbacks.push_back(callback);
		return mComponentAddedCallbacks.end() - 1;
	}

	void unsubscribeComponentAddedEvent(const std::vector<std::function<void(const Entity&)>>::iterator& iterator)
	{
		mComponentAddedCallbacks.erase(iterator);
	}

	std::vector<std::function<void(const Entity& entity)>>::iterator subscribeRemoveEvent(const std::function<void(const Entity&)>& callback)
	{
		mComponentRemovedCallbacks.push_back(callback);
		return mComponentRemovedCallbacks.end() - 1;
	}

	void unsubscribeComponentRemovedEvent(const std::vector<std::function<void(const Entity&)>>::iterator& iterator)
	{
		mComponentRemovedCallbacks.erase(iterator);
	}
};