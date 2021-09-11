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
	ComponentBit bit = 1;
	for (ComponentID componentID = 0; componentID < 64; componentID++)
	{
		if (composition & bit)
			ComponentManagerBase::componentManagerFromID(componentID).removeComponent(*this);
		bit <<= 1;
	}

	compositions.erase(mID);
	queuedIDs.push_back(mID);
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
