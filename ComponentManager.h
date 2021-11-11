#pragma once
#include "Entity.h"
#include <functional>


#include <iostream>

typedef unsigned long long ComponentBit; // 1 bit-shifted by the component ID to indicate the position of flag bit in 'Composition' (See Entity.h)

class ComponentManagerBase
{
private:
	// Incremented on each component manager instantiation, they will not be destroyed until the program terminates because they are singletons
	static ComponentID queuedID;

	// Maps each component ID to polymorphic pointers referencing a corresponding manager instance
	static std::unordered_map<ComponentID, ComponentManagerBase*> IDInstanceMap;

protected:
	ComponentManagerBase();

public:
	static ComponentManagerBase& componentManagerFromID(const ComponentID& ID);

	const ComponentID ID;
	const ComponentBit bit;
	
	~ComponentManagerBase();
	
	virtual void removeComponent(const Entity& entity) = 0;

	virtual std::vector<char> getSerializedComponent(const EntityID& ID) = 0;
	virtual void addSerializedComponent(const std::vector<char>& vecData, Entity& entity) = 0;
};

// There is one ComponentManager instance managing the storage of each type of Component, attatched to entities
template <typename T>
class ComponentManager : public ComponentManagerBase
{
private:
	// Maps all entities possessing the component type to indexes into the contiguous array 'mComponents' hopefully reducing cache misses when iterating over entities
	std::unordered_map<EntityID, unsigned int> mEntityIndexMap;
	std::vector<T> mComponents;

	// Component manager is responsible for notifying all Systems among other things if its respective component type is being added or removed to/from an entity
	std::vector<std::function<void(const Entity&)>> mComponentAddedCallbacks;
	std::vector<std::function<void(const Entity&)>> mComponentRemovedCallbacks;

	ComponentManager() {};

public:
	// Only one instance of each ComponentManager should exist
	static ComponentManager& instance() 
	{
		static ComponentManager instance;
		return instance;
	}
	ComponentManager(const ComponentManager& copy) = delete;

	T& addComponent(const Entity& entity, T component)
	{
		assert(("[ERROR] Cannot add component to entity that already possesses a component of that type", mEntityIndexMap.find(entity.ID()) == mEntityIndexMap.end()));
		mEntityIndexMap.insert({ entity.ID(), mComponents.size() }); // Entity ID now maps to next available index
		mComponents.push_back(component); // Add component at index

		entity.composition() |= bit; // Add component's bit to entity's composition

		// Notify subscribers of 'component added' event
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
		std::unordered_map<EntityID, unsigned int>::iterator IDIterator = mEntityIndexMap.find(entity.ID());
		assert(("[ERROR] Cannot remove component from an entity that does not possess a component of that type", IDIterator != mEntityIndexMap.end()));

		// Notify subscribers of 'component removed' event
		for (const std::function<void(const Entity&)>& callback : mComponentRemovedCallbacks)
			callback(entity);

		entity.composition() &= ~bit; // Remove component's bit from entity's composition
		
		// Find mapping with it's index accessing the last element in 'mComponents' and change the index to now be the index of the component being removed
		for (std::unordered_map<EntityID, unsigned int>::iterator it = mEntityIndexMap.begin(); it != mEntityIndexMap.end(); it++)
		{
			if (it->second == mComponents.size() - 1)
			{
				it->second = IDIterator->second;
				break;
			}
		}

		// No gaps
		mComponents[IDIterator->second] = mComponents.back(); // Overwrite component being removed with last component
		mComponents.pop_back(); // Free last component
		mEntityIndexMap.erase(IDIterator); // Remove entity's ID mapping
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

	// Interface with component added and removed callback arrays
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