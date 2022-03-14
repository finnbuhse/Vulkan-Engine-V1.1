#pragma once
#include "Transform.h"

class SceneManager
{
private:
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();

	SceneManager() {};

public:
	std::vector<EntityID> mSceneEntityIDs;

	static SceneManager& instance();

	void destroy();

	Entity createEntity();
	void destroyEntity(const Entity& entity, const bool& children = true);

	void addEntity(const Entity& entity, const bool& children = true);
	
	void saveScene(const char* filename);
	
	void loadScene(const char* filename, const bool& destroyCurrent = true);
};
