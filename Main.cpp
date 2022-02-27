#include "WindowManager.h"
#include "Mesh.h"
#include "Physics.h"
#include "CameraController.h"
#include "SceneManager.h"
#include "FontManager.h"
#include <chrono>
#include <iostream>

Entity camera;

void toggleCursor()
{
	static WindowManager& windowManager = WindowManager::instance();
	static bool enabled = DEFAULT_DISABLE_CURSOR ? false : true;
	enabled = !enabled;

	CameraController& controller = camera.getComponent<CameraController>();
	if (enabled)
	{
		windowManager.enableCursor();
		controller.pitch = false;
		controller.yaw = false;
	}
	else
	{
		windowManager.disableCursor();
		controller.pitch = true;
		controller.yaw = true;
	}
}

int main()
{
	WindowManager& windowManager = WindowManager::instance();
	KeyCallback toggleCursorCallback(toggleCursor);
	windowManager.subscribeKeyPressEvent(C, &toggleCursorCallback);

	SceneManager& sceneManager = SceneManager::instance();

	TransformSystem& transformSystem = TransformSystem::instance();
	RenderSystem& renderSystem = RenderSystem::instance();
	renderSystem.initialize();
	CameraSystem& cameraSystem = CameraSystem::instance();
	CameraControllerSystem& cameraControllerSystem = CameraControllerSystem::instance();
	PhysicsSystem& physicsSystem = PhysicsSystem::instance();
	
	FontManager& fontManager = FontManager::instance();

	renderSystem.setSkybox(&TextureManager::instance().getCubemap({ { "Images/Skybox/right.hdr", "Images/Skybox/left.hdr", "Images/Skybox/bottom.hdr", "Images/Skybox/top.hdr", "Images/Skybox/front.hdr", "Images/Skybox/back.hdr" }, FORMAT_RGBA_HDR16 }));
	
	camera = sceneManager.createEntity();
	camera.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::quat(0.0f, 0.0f, 0.0f, 1.0f), glm::vec3(1.0f) });
	camera.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.05f, 100.0f });
	camera.addComponent<CameraController>(CameraController{ 0.0f, 0.005f, true, true, true });
	renderSystem.setCamera(camera);

	const Entity FPSCounter = sceneManager.createEntity();
	UIText& FPSText = FPSCounter.addComponent<UIText>(UIText{ glm::vec2(0.875f, -0.9f), 1.0f, "", "Fonts/arial.ttf", glm::vec3(1.0f)});

	// Required for current pipeline
	const Entity directionalLight = sceneManager.createEntity();
	directionalLight.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)) });
	directionalLight.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });

	const Entity dropdown = sceneManager.createEntity();
	dropdown.addComponent<UIButton>(UIButtonCreateInfo{ glm::vec2(0.0f), glm::vec2(0.108f, 0.192f), glm::vec3(1.0f), "Images/Dropdown unpressed.png", "Images/Dropdown canpress.png", "Images/Dropdown pressed.png", toggleCursor });

	std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
	while (!windowManager.windowClosed())
	{
		now = std::chrono::high_resolution_clock::now();
		double deltaTime = std::chrono::duration<double>(now - last).count();
		FPSText.text = std::to_string((unsigned int)(1.0f / deltaTime)) + " FPS";

		transformSystem.update();
		renderSystem.update();
		cameraSystem.update();
		physicsSystem.update(deltaTime);
		cameraControllerSystem.update(deltaTime);

		windowManager.pollEvents();

		last = now;
	}

	sceneManager.destroy();
	return 0;
}