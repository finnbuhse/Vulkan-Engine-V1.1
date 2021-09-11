#pragma once
#include "WindowManager.h"
#include "Transform.h"
#include "glm/glm.hpp"

struct CameraController
{
	float movementSpeed;
	float mouseSensitivity;
};

class CameraControllerSystem
{
private:
	Composition mComposition;
	std::vector<EntityID> mEntityIDs;
	glm::vec2 mLastCursorPosition;

	WindowManager& mWindowManager = WindowManager::instance();
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<CameraController>& mCameraControllerManager = ComponentManager<CameraController>::instance();

	CameraControllerSystem();

public:
	static CameraControllerSystem& instance();

	CameraControllerSystem(const CameraControllerSystem& copy) = delete;

	void componentAdded(const Entity& entity);
	void componentRemoved(const Entity& entity);

	void update(const float& deltaTime);
};