#pragma once
#include "Entity.h"

class SceneManager
{
private:
	

	SceneManager() {};

public:
	std::vector<EntityID> mSceneEntityIDs;

	static SceneManager& instance();

	void destroy();

	Entity createEntity();
	void destroyEntity(const Entity& entity, const bool& children = true);

	void addEntity(const Entity& entity, const bool& children = true);
};