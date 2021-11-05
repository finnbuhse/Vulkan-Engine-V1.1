#include "ComponentManager.h"

std::vector<EntityID> Entity::queuedIDs = { 1 };
std::unordered_map<EntityID, Composition> Entity::compositions;

Composition& Entity::compositionFromID(const EntityID& ID)
{
	return compositions[ID];
}

Entity::Entity() :
	mID(queuedIDs.back())
{
	queuedIDs.pop_back(); // Remove ID from queue
	if (queuedIDs.empty()) // If queue is empty, it is guranteed that all IDs to 'mID' are used
		queuedIDs.push_back(mID + 1); // So next ID assigned should be 'mID' + 1

	compositions.insert({ mID, 0 });
}

Entity::Entity(const EntityID& ID) :
	mID(ID)
{
	assert(("[ERROR] Could not dereference ID", ID && ID < queuedIDs.front()));
	#ifndef NDEBUG
	for (unsigned int i = 1; i < queuedIDs.size(); i++)
		assert(("[ERROR] Could not dereference ID", ID != queuedIDs[i]));
	#endif
}

Entity::Entity(const Entity& copy) : mID(copy.mID) {}

Entity& Entity::operator=(const Entity& other)
{
	return *this;
}

void Entity::destroy() const
{
	// Remove all components from entity
	Composition composition = compositions[mID];
	for (ComponentID componentID = 0; componentID < 64; componentID++)
	{
		if ((composition >> componentID) & 1)
			ComponentManagerBase::componentManagerFromID(componentID).removeComponent(*this);
	}

	compositions.erase(mID); // Remove entity's composition entry in map
	queuedIDs.push_back(mID); // Add ID to queue to be reused
}

bool Entity::operator==(const Entity& other) const
{
	return mID == other.mID;
}

const EntityID Entity::ID() const
{
	return mID;
}

Composition& Entity::composition() const
{
	return compositions[mID];
}

unsigned int Entity::numberOfComponents() const
{
	unsigned int nComponents = 0;
	Composition composition = compositions[mID];
	for (ComponentID componentID = 0; componentID < 64; componentID++)
	{
		if ((composition >> componentID) & 1)
			nComponents++;
	}
	return nComponents;
}

template<>
std::vector<char> serialize(const Entity& entity)
{
	std::vector<char> result;
	
	std::vector<char> vecData = serialize(entity.numberOfComponents());
	result.insert(result.end(), vecData.begin(), vecData.end());
	
	for (ComponentID componentID = 0; i < 64; i++)
	{
		if((composition >> componentID) & 1)
		{
			vecData = serialize(componentID);
			vecData = ComponentManagerBase::getComponentManager(componentID).getSerializedComponent(entity);
			result.insert(result.end(), vecData.begin(), vecData.end());
		}
	}
	
	if(entity.hasComponent<Transform>())
	{
		Transform& transform = entity.getComponent<Transform>();
		vecData = serialize(transform.children.length);
		
		for (unsigned int i = 0; i < transform.children.length; i++)
		{
			vecData = serialize(transform.children[i].entity);
			result.insert(result.end(), vecData.begin(), vecData.end());
		}
	}
	
	return result;
}

template <>
void deserialize(const std::vector<char>& vecData, Entity& entity)
{
	unsigned int begin = 0;
	unsigned int size = sizeof(unsigned int);
	
	unsigned int nComponents = deserialize(vecData.data(), vecData.data() + size);
	begin += size;
	
	for (unsigned int i = 0; i < nComponents; i++)
	{
		size = sizeof(unsigned int);
		ComponentID componentID = deserialize(vecData.data() + begin, vecData.data() + begin + size);
		begin += size;
		
		if ComponentManager<Transform>::instance().ID():
		{
			Transform& transform = entity.addComponent<Transform>();
			TransformCreateInfo transformCreateInfo;
			deserialize(std::vector<char>(vecData.data() + begin, vecData.data() + begin + size), transformCreateInfo);
			transform = transformCreateInfo;
		}
	}
	
	//....
}
