#include "SceneManager.h"
#include "Transform.h"

SceneManager& SceneManager::instance()
{
	static SceneManager instance;
	return instance;
}

void SceneManager::destroyScene()
{
	for (const EntityID& entityID : mSceneEntityIDs)
	{
		Entity entity(entityID);
		entity.destroy();
	}
}

Entity SceneManager::createEntity()
{
	Entity entity;
	mSceneEntityIDs.push_back(entity.ID());
	return entity;
}

void SceneManager::destroyEntity(const Entity& entity, const bool& children)
{
	std::vector<EntityID>::iterator it = std::find(mSceneEntityIDs.begin(), mSceneEntityIDs.end(), entity.ID());
	if (it != mSceneEntityIDs.end())
		mSceneEntityIDs.erase(it);
	if (children)
	{
		Transform* transform = &entity.getComponent<Transform>();
		while (transform->childrenIDs.length != 0)
		{
			destroyEntity(Entity(transform->childrenIDs[0]));
			transform = &entity.getComponent<Transform>();
		}
	}
	entity.destroy();
}

void SceneManager::addEntity(const Entity& entity, const bool& children)
{
	mSceneEntityIDs.push_back(entity.ID());
	if (children)
	{
		Transform& transform = entity.getComponent<Transform>();
		for (unsigned int i = 0; i < transform.childrenIDs.length; i++)
			addEntity(Entity(transform.childrenIDs[i]));
	}
}

void SceneManager::saveScene(const char* filename)
{
	std::vector<char> result;
	std::vector<char> data;
	
	unsigned int nParentEntities;
	for (unsigned int i = 0; i < mSceneEntityIDs.size(); i++)
	{
		Transform& transform = mTransformManager.getComponent(mSceneEntityIDs[i]);
		if(transform.parentID == 0)
		{
			data = serialize(Entity(mSceneEntityIDs[i]));
			result.insert(result.end(), data.begin(), data.end());
			nParentEntities++;
		}
	}
	
	data = serialize(nParentEntities);
	result.insert(result.begin(), data.begin(), data.end());
	
	writeFile(filename, result);
}

void SceneManager::loadScene(const char* filename, const bool& destroyCurrent)
{
	std::vector<char> scene = readFile(filename);
}
