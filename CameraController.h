#pragma once
#include "WindowManager.h"
#include "Transform.h"
#include "glm/glm.hpp"

struct CameraController
{
	float movementSpeed;
	float mouseSensitivity;
	bool movement;
	bool pitch;
	bool yaw;
};

class CameraControllerSystem
{
private:
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<CameraController>& mCameraControllerManager = ComponentManager<CameraController>::instance();
	WindowManager& mWindowManager = WindowManager::instance();

	const ComponentAddedCallback mComponentAddedCallback = std::bind(&CameraControllerSystem::componentAdded, this, std::placeholders::_1);
	const ComponentRemovedCallback mComponentRemovedCallback = std::bind(&CameraControllerSystem::componentRemoved, this, std::placeholders::_1);

	Composition mComposition;
	std::vector<EntityID> mEntityIDs;

	glm::vec2 mLastCursorPosition;

	CameraControllerSystem();

public:
	static CameraControllerSystem& instance();

	CameraControllerSystem(const CameraControllerSystem& copy) = delete;
	~CameraControllerSystem();

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void update(const double& deltaTime);
};