#include "CameraController.h"
#include "Camera.h"

CameraControllerSystem::CameraControllerSystem()
{
	mLastCursorPosition = mWindowManager.cursorPosition();

	ComponentManager<Camera>& cameraManager = ComponentManager<Camera>::instance();
	
	mComposition = mTransformManager.bit | cameraManager.bit | mCameraControllerManager.bit;

	mTransformManager.subscribeAddEvent(std::bind(&CameraControllerSystem::componentAdded, this, std::placeholders::_1));
	mTransformManager.subscribeRemoveEvent(std::bind(&CameraControllerSystem::componentRemoved, this, std::placeholders::_1));
	cameraManager.subscribeAddEvent(std::bind(&CameraControllerSystem::componentAdded, this, std::placeholders::_1));
	cameraManager.subscribeRemoveEvent(std::bind(&CameraControllerSystem::componentRemoved, this, std::placeholders::_1));
	mCameraControllerManager.subscribeAddEvent(std::bind(&CameraControllerSystem::componentAdded, this, std::placeholders::_1));
	mCameraControllerManager.subscribeRemoveEvent(std::bind(&CameraControllerSystem::componentRemoved, this, std::placeholders::_1));
}

CameraControllerSystem& CameraControllerSystem::instance()
{
	static CameraControllerSystem instance;
	return instance;
}

void CameraControllerSystem::componentAdded(const Entity& entity)
{
	if ((entity.composition() & mComposition) == mComposition)
		mEntityIDs.push_back(entity.ID());
}

void CameraControllerSystem::componentRemoved(const Entity& entity)
{
	std::vector<EntityID>::iterator iterator = std::find(mEntityIDs.begin(), mEntityIDs.end(), entity.ID());
	if (iterator != mEntityIDs.end())
		mEntityIDs.erase(iterator);
}

void CameraControllerSystem::update(const float& deltaTime)
{
	bool wDown = mWindowManager.keyDown(W);
	bool sDown = mWindowManager.keyDown(S);
	bool aDown = mWindowManager.keyDown(A);
	bool dDown = mWindowManager.keyDown(D);

	glm::vec2 cursorPosition = mWindowManager.cursorPosition();
	glm::vec2 cursorDelta = cursorPosition - mLastCursorPosition;
	mLastCursorPosition = cursorPosition;

	// Input controls all CameraControllers in the scene
	for (const EntityID& ID : mEntityIDs)
	{
		Transform& transform = mTransformManager.getComponent(ID);
		CameraController& cameraController = mCameraControllerManager.getComponent(ID);

		if(wDown)
			transform.translate(transform.worldDirection() * cameraController.movementSpeed * deltaTime);
		if(sDown)
			transform.translate(transform.worldDirection(glm::vec3(0.0f, 0.0f, 1.0f)) * cameraController.movementSpeed * deltaTime);
		if(aDown)
			transform.translate(transform.worldDirection(glm::vec3(-1.0f, 0.0f, 0.0f)) * cameraController.movementSpeed * deltaTime);
		if (dDown)
			transform.translate(transform.worldDirection(glm::vec3(1.0f, 0.0f, 0.0f)) * cameraController.movementSpeed * deltaTime);

		transform.rotate(cursorDelta.y * cameraController.mouseSensitivity, transform.worldDirection(glm::vec3(1.0f, 0.0f, 0.0f)));
		transform.rotate(-cursorDelta.x * cameraController.mouseSensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
	}
}
