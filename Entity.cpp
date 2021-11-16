#include "ComponentManager.h"
#include "Transform.h"

std::vector<EntityID> Entity::queuedIDs = { 1 };
std::unordered_map<EntityID, Composition> Entity::compositions;

Composition& Entity::getCompositionFromID(const EntityID& ID)
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

unsigned int Entity::nbComponents() const
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

	std::vector<char> vecData = serialize(entity.nbComponents());
	result.insert(result.end(), vecData.begin(), vecData.end());

	Composition entityComposition = entity.composition();

	for (ComponentID componentID = 0; componentID < 64; componentID++)
	{
		if ((entityComposition >> componentID) & 1)
		{
			vecData = serialize(componentID);
			result.insert(result.end(), vecData.begin(), vecData.end());
			vecData = ComponentManagerBase::componentManagerFromID(componentID).getSerializedComponent(entity.ID());
			std::vector<char> tempVecData = serialize((unsigned int)vecData.size());
			result.insert(result.end(), tempVecData.begin(), tempVecData.end());
			result.insert(result.end(), vecData.begin(), vecData.end());
		}
	}

	if (entity.hasComponent<Transform>())
	{
		Transform& transform = entity.getComponent<Transform>();
		vecData = serialize(transform.childrenIDs.length);
		result.insert(result.end(), vecData.begin(), vecData.end());

		for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
		{
			vecData = serialize(Entity(transform.childrenIDs[i]));
			result.insert(result.end(), vecData.begin(), vecData.end());
		}
	}
	else
	{
		vecData = serialize(0U);
		result.insert(result.end(), vecData.begin(), vecData.end());
	}
	return result;
}

template <>
void deserialize(const std::vector<char>& vecData, Entity& entity)
{
	unsigned int begin = 0;
	unsigned int size = sizeof(unsigned int);

	unsigned int nComponents;
	deserialize(std::vector<char>(vecData.data(), vecData.data() + size), nComponents);
	begin += size;

	for (unsigned int i = 0; i < nComponents; i++)
	{
		size = sizeof(unsigned int);

		ComponentID componentID;
		deserialize(std::vector<char>(vecData.data() + begin, vecData.data() + begin + size), componentID);
		begin += size;

		unsigned int componentSize;
		deserialize(std::vector<char>(vecData.data() + begin, vecData.data() + begin + size), componentSize);
		begin += size;

		size = componentSize;
		ComponentManagerBase::componentManagerFromID(componentID).addSerializedComponent(std::vector<char>(vecData.data() + begin, vecData.data() + begin + size), entity);
		begin += size;
	}

	size = sizeof(unsigned int);

	unsigned int nChildren;
	deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), nChildren);
	begin += size;

	for (unsigned int i = 0; i < nChildren; i++)
	{
		Entity child;
		deserialize(std::vector<char>(vecData.begin() + begin, vecData.end()), child);
	}
}