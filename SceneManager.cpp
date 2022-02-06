#include "SceneManager.h"
#include "Transform.h"

SceneManager& SceneManager::instance()
{
	static SceneManager instance;
	return instance;
}

void SceneManager::destroy()
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
