#pragma once
#include <vector>
#include <unordered_map>

typedef unsigned int EntityID;
typedef unsigned int ComponentID; // Unique ID assinged to each type of Component
typedef unsigned long long Composition; // Set of flags describing a set of components, limited to 64 component types

template <typename T>
class ComponentManager;

class Entity
{
private:
	template <typename T>
	friend class ComponentManager;

	static std::vector<EntityID> queuedIDs; // Queue of pending IDs to be assigned to new entities. 0 represents NULL ID, first valid ID is 1
	static std::unordered_map<EntityID, Composition> compositions; // ID to Compositions map describing what components each entity possesses

	EntityID mID;

public:
	// Returns an entity's composition from its ID
	static Composition& compositionFromID(const EntityID& ID); 

	Entity();

	/* Because an Entity's members consist of just an ID, assignment operations simply copy the ID; making the entity reference different resources.
	   However if you do this, much like manually allocated arrays ensure the original entity is not 'lost' and will be freed exactly once. */

	Entity(const EntityID& ID); 
	Entity(const Entity& copy);
	Entity& operator =(const Entity& other);

	void destroy() const;
	
	// Returns true if IDs are equal meaning they are the same entity
	bool operator ==(const Entity& other) const;

	// Returns the entity's unique ID
	const EntityID ID() const;

	// Returns the entity's composition
	Composition& composition() const; 

	/*
	Adds a component of type T to the entity. Only one component of each type may be added to an entity
	\param component: The component to be added. Data is copied to a new component stored internally
	\return A reference to the newly created component
	*/
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

	unsigned int numberOfComponents() const;
};