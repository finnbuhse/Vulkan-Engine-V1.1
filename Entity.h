#pragma once
#include <vector>
#include <unordered_map>

typedef unsigned int EntityID;
typedef unsigned long long Composition; // Maximum of 64 Component types

template <typename T>
class ComponentManager;

class Entity
{
private:
	template <typename T>
	friend class ComponentManager;

	static std::vector<EntityID> queuedIDs; // 0 is NULL entity, first valid ID is 1
	static std::unordered_map<EntityID, Composition> compositions;

	EntityID mID;

public:
	static Composition& compositionFromID(const EntityID& ID);

	Entity();
	Entity(const EntityID& ID);
	Entity(const Entity& copy);
	Entity& operator =(const Entity& other);

	void destroy() const;
	
	bool operator ==(const Entity& other) const;

	const EntityID ID() const;
	Composition& composition() const;

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
};