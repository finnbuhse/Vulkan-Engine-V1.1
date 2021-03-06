#pragma once
#include "Entity.h"
#include <functional>

typedef unsigned long long ComponentBit;
typedef std::function<void(const Entity&)> ComponentAddedCallback;
typedef std::function<void(const Entity&)> ComponentRemovedCallback;

class ComponentManagerBase
{
private:
	static ComponentID queuedID;

	static std::unordered_map<ComponentID, ComponentManagerBase*> IDInstanceMap;

protected:
	ComponentManagerBase(const char* componentName);

public:
	static ComponentManagerBase& componentManagerFromID(const ComponentID& ID);
	
	const ComponentID ID; // The unique ID identifying the managers component type
	const ComponentBit bit; // The position of the bit in a Composition bitmask indicating possession of the component type
	const char* componentName;

	~ComponentManagerBase();
	
	/*
	Removes component from an entity.
	\param Entity: Entity to remove the component from.
	*/
	virtual void removeComponent(const Entity& entity) = 0;

	/*
	Retrieves component and serializes it.
	\param ID: ID of the entity to get component from.
	\return: The serialized component.
	*/
	virtual std::vector<char> getSerializedComponent(const EntityID& ID) = 0;
	
	/*
	Deserializes a serialized component and adds it to an entity.
	\param vecData: The serialized component.
	\param entity: Entity to add the component to.
	*/
	virtual void addSerializedComponent(const std::vector<char>& vecData, Entity& entity) = 0;
};

template <typename T>
class ComponentManager : public ComponentManagerBase
{
private:
	std::unordered_map<EntityID, unsigned int> mEntityIndexMap; // Maps entity IDs to indices accessing mComponents
	std::vector<T> mComponents; // Dynamic array containing all components of type T

	// Dynamic arrays containing references to callbacks to be invoked when a component of type T is added or removed 
	std::vector<ComponentAddedCallback*> mComponentAddedCallbacks;
	std::vector<ComponentRemovedCallback*> mComponentRemovedCallbacks;

	ComponentManager() : ComponentManagerBase(typeid(T).name()) {};

public:
	static ComponentManager& instance() 
	{
		static ComponentManager instance;
		return instance;
	}

	ComponentManager(const ComponentManager& copy) = delete;

	/*
	Adds a component of type T to the entity.
	\param entity: Entity to add the component to.
	\param component: Component to add.
	\return A reference to the newly added component.
	*/
	T& addComponent(const Entity& entity, T component)
	{
		assert(("[ERROR] Cannot add component to entity that already possesses a component of that type", mEntityIndexMap.find(entity.ID()) == mEntityIndexMap.end()));

		mEntityIndexMap.insert({ entity.mID, mComponents.size() }); // Entity ID now relates to the next free index
		mComponents.push_back(component); // Add component at index

		Entity::compositions[entity.mID] |= bit; // Add component's bit to entity's composition

		// Notify subscribers of 'component added' event
		for (ComponentAddedCallback* callback : mComponentAddedCallbacks)
			(*callback)(entity);

		return mComponents.back();
	}

	/*
	Gets component of type T from the entity.
	\param entityID: ID of entity to retrieve component from.
	\return A reference to the component.
	*/
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

	/*
	Removes component of type T from the entity.
	\param entity: Entity to remove the component from.
	*/
	void removeComponent(const Entity& entity) override
	{
		// Find entity in entity index map
		std::unordered_map<EntityID, unsigned int>::iterator IDIterator = mEntityIndexMap.find(entity.ID());
		assert(("[ERROR] Cannot remove component from an entity that does not possess a component of that type", IDIterator != mEntityIndexMap.end()));

		// Notify subscribers of 'component removed' event
		for (ComponentRemovedCallback* callback : mComponentRemovedCallbacks)
			(*callback)(entity);

		Entity::compositions[entity.mID] &= ~bit; // Remove component's bit from entity's composition
		
		// Find relation where it's index accesses the last element in 'mComponents', and assign the index to be the index of the component being removed
		for (std::unordered_map<EntityID, unsigned int>::iterator it = mEntityIndexMap.begin(); it != mEntityIndexMap.end(); it++)
		{
			if (it->second == mComponents.size() - 1)
			{
				it->second = IDIterator->second;
				break;
			}
		}
		mComponents[IDIterator->second] = mComponents.back(); // Overwrite component being removed with last component. Ensures no gaps

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

	/*
	Add a procedure to get automatically invoked whenever a component of type T is added to an entity.
	\param callback: Pointer to the procedure to be added. Procedure must follow template: void [procedure name](const Entity& [entity name]).
	*/
	void subscribeAddedEvent(const ComponentAddedCallback* callback)
	{
		mComponentAddedCallbacks.push_back((ComponentRemovedCallback*)callback);
	}

	/*
	Remove a procedure from the array of procedures that get invoked once a component of type T is added.
	\param callback: Pointer to the procedure to be removed.
	*/
	void unsubscribeAddedEvent(const ComponentAddedCallback* callback)
	{
		mComponentAddedCallbacks.erase(std::find(mComponentAddedCallbacks.begin(), mComponentAddedCallbacks.end(), (ComponentRemovedCallback*)callback));
	}

	/*
	Add a procedure to get automatically invoked whenever a component of type T is removed from entity.
	\param callback: Pointer to the procedure to be added. Procedure must follow template: void [procedure name](const Entity& [entity name]).
	*/
	void subscribeRemovedEvent(const ComponentRemovedCallback* callback)
	{
		mComponentRemovedCallbacks.push_back((ComponentRemovedCallback*)callback);
	}

	/*
	Remove a procedure from the array of procedures that get invoked once a component of type T is removed.
	\param callback: Pointer to the procedure to be removed.
	*/
	void unsubscribeRemovedEvent(const ComponentRemovedCallback* callback)
	{
		mComponentRemovedCallbacks.erase(std::find(mComponentRemovedCallbacks.begin(), mComponentRemovedCallbacks.end(), (ComponentRemovedCallback*)callback));
	}
};