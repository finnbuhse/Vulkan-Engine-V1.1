#pragma once
#include <vector>
#include <unordered_map>

typedef unsigned int EntityID; // Unique ID assigned to each entity
typedef unsigned long long Composition; // Set of flags indicating a set of components, unsigned long long limits the engine to a maximum of 64 Component types

template <typename T>
class ComponentManager;

class Entity
{
private:
	template <typename T>
	friend class ComponentManager;

	static std::vector<EntityID> queuedIDs; // Queue of pending IDs to be assigned to new entities. 0 represents NULL ID, first valid ID is 1
	static std::unordered_map<EntityID, Composition> compositions; // ID to Compositions map describing what components each entity possesses

	EntityID mID; // Entity's unique ID; only data member data making storage of both EntityIDs (Contiguous) and Entities (Not guranteed contiguous) very efficient aswell as assignment operations

public:
	// Retrieve composition from entites ID
	static Composition& compositionFromID(const EntityID& ID); 

	Entity();

	/* Due to the fact that an Entity is simply a Number data-wise, it can be assigned to a different entity becoming a handle to different resources behaving like a reference.
	   However if you do this ensure the original entity is not 'lost' and will eventually be freed.
	   I have implemented a 'sceneManager' in the past which allowed for automatic loading and destruction of entities however that is not present anymore
	   and so is currently manual. Explicit destruction is employed mainly to enable this flexibility */
	Entity(const EntityID& ID); 
	Entity(const Entity& copy);
	Entity& operator =(const Entity& other);

	void destroy() const;
	
	// If IDs are equal meaning they are both reference the exact same entity
	bool operator ==(const Entity& other) const;

	const EntityID ID() const;
	Composition& composition() const; 

	// See 'ComponentManager.h', easiest interface to add, get, and remove components from entities
	template <typename T>
	T& addComponent(T component) const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		return componentManager.addComponent(*this, component);
	}

	template <typename T>
	T& getComponent() const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		return componentManager.getComponent(mID);
	}

	template <typename T>
	void removeComponent() const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		componentManager.removeComponent(*this);
	}

	template <typename T>
	bool hasComponent() const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		return (composition() & componentManager.bit) == componentManager.bit;
	}
};