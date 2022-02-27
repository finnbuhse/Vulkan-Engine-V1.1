#include "CameraController.h"
#include "Camera.h"

CameraControllerSystem::CameraControllerSystem()
{
	mTransformManager.subscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	mCameraControllerManager.subscribeAddedEvent(&mComponentAddedCallback);
	mCameraControllerManager.subscribeRemovedEvent(&mComponentRemovedCallback);
	
	mComposition = mTransformManager.bit | mCameraControllerManager.bit;

	mLastCursorPosition = mWindowManager.cursorPosition();
}

CameraControllerSystem& CameraControllerSystem::instance()
{
	static CameraControllerSystem instance;
	return instance;
}

CameraControllerSystem::~CameraControllerSystem()
{
	mTransformManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mTransformManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);
	mCameraControllerManager.unsubscribeAddedEvent(&mComponentAddedCallback);
	mCameraControllerManager.unsubscribeRemovedEvent(&mComponentRemovedCallback);
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

void CameraControllerSystem::update(const double& deltaTime)
{
	bool wDown = mWindowManager.keyDown(W);
	bool sDown = mWindowManager.keyDown(S);
	bool aDown = mWindowManager.keyDown(A);
	bool dDown = mWindowManager.keyDown(D);

	glm::vec2 cursorPosition = mWindowManager.cursorPosition();
	glm::vec2 cursorDelta = cursorPosition - mLastCursorPosition;
	mLastCursorPosition = cursorPosition;

	// Input controls all CameraControllers in the scene.
	for (const EntityID& ID : mEntityIDs)
	{
		Transform& transform = mTransformManager.getComponent(ID);
		CameraController& cameraController = mCameraControllerManager.getComponent(ID);

		if (cameraController.movement)
		{
			if (wDown)
				transform.translate(transform.worldDirection() * cameraController.movementSpeed * (float)deltaTime);
			if (sDown)
				transform.translate(transform.worldDirection(glm::vec3(0.0f, 0.0f, 1.0f)) * cameraController.movementSpeed * (float)deltaTime);
			if (aDown)
				transform.translate(transform.worldDirection(glm::vec3(-1.0f, 0.0f, 0.0f)) * cameraController.movementSpeed * (float)deltaTime);
			if (dDown)
				transform.translate(transform.worldDirection(glm::vec3(1.0f, 0.0f, 0.0f)) * cameraController.movementSpeed * (float)deltaTime);
		}

		if (cameraController.pitch)
			transform.rotate(cursorDelta.y * cameraController.mouseSensitivity, transform.direction(glm::vec3(1.0f, 0.0f, 0.0f)));
			
		if(cameraController.yaw)
			transform.rotate(-cursorDelta.x * cameraController.mouseSensitivity, glm::vec3(0.0f, 1.0f, 0.0f));
	}
}
