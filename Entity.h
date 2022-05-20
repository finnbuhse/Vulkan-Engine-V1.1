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

	// Dynamic array containing available IDs to use for new entties
	static std::vector<EntityID> freeIDStack;

	// Maps each entity ID to a Composition indicating which components each entity has
	static std::unordered_map<EntityID, Composition> compositions;

	// Maps each entity ID to a name for the entity
	static std::unordered_map<EntityID, std::string> names;

	EntityID mID; // Unique number identifying the entity

public:
	/*
	Get the composition of an entity via it's ID.
	\param ID: An ID identifying an existing entity.
	\return A reference to the entity's composition.
	*/
	static Composition& getCompositionFromID(const EntityID& ID);

	static std::string& getNameFromID(const EntityID& ID);

	Entity(const std::string& name = "");
	
	/*
	Construct an entity from an ID.
	\param ID: An ID identifying an existing entity. The created entity becomes equivilent to the entity identified by ID. 
	*/
	Entity(const EntityID& ID);

	/*
	Construct an entity from an existing entity.
	\param entity: An existing entity. The created entity references the same components as the inputed entity and is not a copy/duplicate but a new reference to the same entity.
	*/
	Entity(const Entity& entity);

	Entity& operator =(const Entity& other);

	/*
	Releases the entity's components. All created entities should be destroyed before the program terminates.
	*/
	void destroy();

	bool operator ==(const Entity& other) const;

	/*
	\return The unique ID of the entity.
	*/
	const EntityID ID() const;

	/*
	Gets the composition of the entity. The Composition type is a bitmask indicating which components an entity possesses.
	\return A reference to the entity's composition.
	*/
	const Composition& composition() const;

	/*
	Sets the name of the entity.
	\param name: Name to assign to the entity.
	*/
	void setName(const std::string& name);

	/*
	\returns The name of the entity.
	*/
	const std::string& name() const;

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

/*
Recursive method which serializes an entity followed by serializing each of its children.
\param entity: Entity to serialize.
\return An array of bytes containing the serialized entity.
*/
template <>
std::vector<char> serialize(const Entity& entity);

/*
Recursive method which deserializes an entity followed by deserializing each of its children. 
Note potentially produces entity with children, access children via Transform component. Free them using the destroyChildren method.
\param vecData: Serialized entity in byte form.
\param entity: A new entity to write to.
*/
template <>
void deserialize(const std::vector<char>& vecData, Entity& entity);

/*
Recursive method which destroys all children and children of children from an entity. Allows you to free a whole heirarchy just by caching the parent, 
effectively using transforms to maintain references to the children.
\param entity : The entity whos children to destroy.
*/
void destroyChildren(const Entity& entity);