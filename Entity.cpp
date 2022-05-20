#include "ComponentManager.h"
#include "Transform.h"

std::vector<EntityID> Entity::freeIDStack = { 1 };
std::unordered_map<EntityID, Composition> Entity::compositions;
std::unordered_map<EntityID, std::string> Entity::names;

Composition& Entity::getCompositionFromID(const EntityID& ID)
{
	return compositions.at(ID);
}

std::string& Entity::getNameFromID(const EntityID& ID)
{
	return names.at(ID);
}

Entity::Entity(const std::string& name) :
	mID(freeIDStack.back())
{
	freeIDStack.pop_back(); // Remove ID from queue

	if (freeIDStack.empty()) // If queue is empty, it is guranteed that all IDs to 'mID' are used
		freeIDStack.push_back(mID + 1); // So next ID assigned should be 'mID' + 1

	compositions.insert({ mID, 0 });
	names.insert({ mID, name == "" ? "Entity " + std::to_string(mID) : name });
}

Entity::Entity(const EntityID& ID) :
	mID(ID)
{
	assert(("[ERROR] Could not dereference ID", ID && ID < freeIDStack.front()));
	#ifndef NDEBUG
	for (unsigned int i = 1; i < freeIDStack.size(); i++)
		assert(("[ERROR] Could not dereference ID", ID != freeIDStack[i]));
	#endif
}

Entity::Entity(const Entity& copy) : mID(copy.mID) {}

Entity& Entity::operator=(const Entity& other)
{
	return *this;
}

void Entity::destroy()
{
	// Remove all components from entity
	Composition composition = compositions[mID];
	for (ComponentID componentID = 0; componentID < 64; componentID++)
	{
		if ((composition >> componentID) & 1)
		{
			ComponentManagerBase::componentManagerFromID(componentID).removeComponent(*this);
		}
	}

	// Remove entity's entries in maps
	compositions.erase(mID); 
	names.erase(mID);

	freeIDStack.push_back(mID); // Add ID to queue to be reused
	mID = 0;
}

bool Entity::operator==(const Entity& other) const
{
	return mID == other.mID;
}

const EntityID Entity::ID() const
{
	return mID;
}

const Composition& Entity::composition() const
{
	return compositions[mID];
}

void Entity::setName(const std::string& name)
{
	names[mID] = name;
}

const std::string& Entity::name() const
{
	return names[mID];
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
			std::vector<char> tempVecData = serialize((unsigned int)vecData.size());
			result.insert(result.end(), tempVecData.begin(), tempVecData.end());
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
		size = sizeof(unsigned int);

		unsigned int entitySize;
		deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), entitySize);
		begin += size;

		size = entitySize;

		Entity child;
		deserialize(std::vector<char>(vecData.begin() + begin, vecData.begin() + begin + size), child);
		begin += size;

		entity.getComponent<Transform>().addChild(child);
	}
}

void destroyChildren(const Entity& entity)
{
	if (entity.hasComponent<Transform>())
	{
		Transform& transform = entity.getComponent<Transform>();
		while (transform.childrenIDs.length != 0)
		{
			Entity child(transform.childrenIDs[0]);
			destroyChildren(child);
			child.destroy();

			transform = entity.getComponent<Transform>();
		}
	}
	else if (entity.hasComponent<Transform2D>())
	{
		Transform2D& transform = entity.getComponent<Transform2D>();
		while (transform.childrenIDs.length != 0)
		{
			Entity child(transform.childrenIDs[0]);
			destroyChildren(child);
			child.destroy();

			transform = entity.getComponent<Transform2D>();
		}
	}
}
