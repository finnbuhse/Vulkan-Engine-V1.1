#pragma once
#include "Entity.h"
#include <functional>
#include <cassert>

typedef unsigned int ComponentID;
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

	const ComponentID ID;
	const ComponentBit bit;
	
	~ComponentManagerBase();
	
	virtual void removeComponent(const Entity& entity) = 0;
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
		
		// Find mapping with it's index accessing the last element in 'mComponents' and assign it's index to now be the index of the component being removed
		for (std::unordered_map<EntityID, unsigned int>::iterator it = mEntityIndexMap.begin(); it != mEntityIndexMap.end(); it++)
		{
			if (it->second == mComponents.size() - 1)
			{
				it->second = IDIterator->second;
				break;
			}
		}
		mComponents[IDIterator->second] = mComponents.back(); // Move last component to the index of the component being removed
		mEntityIndexMap.erase(IDIterator);
		mComponents.pop_back();
	}

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