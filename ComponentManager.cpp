#include "ComponentManager.h"

ComponentID ComponentManagerBase::queuedID = 0;
std::unordered_map<ComponentID, ComponentManagerBase*> ComponentManagerBase::IDInstanceMap;

ComponentManagerBase::ComponentManagerBase(const char* componentName) :
	componentName(componentName), ID(queuedID), bit(1ULL << queuedID)
{
	IDInstanceMap.insert({ ID, this });
	queuedID++;
}

ComponentManagerBase& ComponentManagerBase::componentManagerFromID(const ComponentID& ID)
{
	return *IDInstanceMap.at(ID);
}

ComponentManagerBase::~ComponentManagerBase()
{
	IDInstanceMap.erase(ID);
}
