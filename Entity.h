#pragma once
#include "Vulkan.h"
#include <unordered_map>

typedef unsigned int EntityID;
typedef unsigned int ComponentID;
typedef unsigned long long Composition;

template <typename T>
class ComponentManager;

class Entity
{
private:
	template <typename T>
	friend class ComponentManager;

	static std::vector<EntityID> queuedIDs;
	static std::unordered_map<EntityID, Composition> compositions;

	EntityID mID;

public:
	/*
	Get the composition of an entity via it's ID.
	\param ID: An ID identifying an existing entity.
	\return A reference to the entity's composition.
	*/
	static Composition& getCompositionFromID(const EntityID& ID);

	Entity();
	
	/*
	Construct an entity from an ID.
	\param ID: An ID identifying an existing entity. The created entity becomes equivilent to the entity identified by ID. 
	*/
	Entity(const EntityID& ID);

	/*
	Construct an entity from an existing entity.
	\param copy: An already existing entity. The created entity becomes equivilent to copy, as entities behave like references.
	*/
	Entity(const Entity& copy);

	Entity& operator =(const Entity& other);

	/*
	Releases the entity's components. All created entities should be destroyed before the program terminates.
	*/
	void destroy() const;

	bool operator ==(const Entity& other) const;

	/*
	\return The unique ID of the entity.
	*/
	const EntityID ID() const;

	/*
	Gets the composition of the entity. The Composition type is a bitmask indicating which components an entity possesses.
	\return A reference to the entity's composition. Manually changing an entity's composition is not advised so copying to a seperate Composition is recommended.
	*/
	Composition& composition() const;

	/*
	Adds a component of type T to the entity. Only one component of each type may be added to an entity.
	\param component: The component to be added. It's data is copied to a new component stored internally.
	\return A reference to the newly created component.
	*/
	template <typename T>
	T& addComponent(T component) const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		return componentManager.addComponent(*this, component);
	}

	/*
	Gets the entity's component of type T.
	\return A reference to the internally stored component.
	*/
	template <typename T>
	T& getComponent() const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		return componentManager.getComponent(mID);
	}

	/*
	Removes component of type T from the entity.
	*/
	template <typename T>
	void removeComponent() const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		componentManager.removeComponent(*this);
	}

	/*
	Use to determine if the entity has a specific component type.
	\return True if the entity possesses a component of type T, otherwise false.
	*/
	template <typename T>
	bool hasComponent() const
	{
		static ComponentManager<T>& componentManager = ComponentManager<T>::instance();
		return (composition() & componentManager.bit) == componentManager.bit;
	}

	/*
	\return The number of components the entity possesses.
	*/
	unsigned int nbComponents() const;
};

template <>
std::vector<char> serialize(const Entity& entity);

template <>
void deserialize(const std::vector<char>& vecData, Entity& entity);