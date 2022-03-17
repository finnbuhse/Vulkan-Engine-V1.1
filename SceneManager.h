#pragma once
#include "Transform.h"
#include <functional>

typedef std::function<void(const Entity& entity)> EntityAddedCallback;
typedef std::function<void(const Entity& entity)> EntityRemovedCallback;

class SceneManager
{
private:
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();

	std::vector<EntityID> mSceneEntityIDs;
	std::vector<EntityAddedCallback*> mEntityAddedCallbacks;
	std::vector<EntityRemovedCallback*> mEntityRemovedCallbacks;

	SceneManager() {};

public:
	static SceneManager& instance();

	void destroyScene();

	Entity createEntity(const std::string& name = "");

	void addEntity(const Entity& entity, const bool& children = true);
	void removeEntity(Entity& entity, const bool& children = true);

	void saveScene(const char* filename) const;
	void loadScene(const char* filename, const bool& destroyCurrent = true);

	void subscribeEntityAddedEvent(EntityAddedCallback* callback);
	void unsubscribeEntityAddedEvent(EntityAddedCallback* callback);

	void subscribeEntityRemovedEvent(EntityRemovedCallback* callback);
	void unsubscribeEntityRemovedEvent(EntityRemovedCallback* callback);
};