#include "ComponentManager.h"

ComponentID ComponentManagerBase::queuedID = 0;
std::unordered_map<ComponentID, ComponentManagerBase*> ComponentManagerBase::IDInstanceMap;

ComponentManagerBase::ComponentManagerBase() :
	ID(queuedID), bit(1ULL << queuedID)
{
	IDInstanceMap.insert({ ID, this });
	queuedID++;
}

ComponentManagerBase& ComponentManagerBase::componentManagerFromID(const ComponentID& ID)
{
	return *IDInstanceMap[ID];
}

ComponentManagerBase::~ComponentManagerBase()
{
	IDInstanceMap.erase(ID);
}
