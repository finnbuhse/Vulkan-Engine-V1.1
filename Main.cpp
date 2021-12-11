#include "WindowManager.h"
#include "Mesh.h"
#include "CameraController.h"
#include "Physics.h"
#include <chrono>
#include <iostream>

int main()
{
	WindowManager& windowManager = WindowManager::instance();

	TransformSystem& transformSystem = TransformSystem::instance();
	RenderSystem& renderSystem = RenderSystem::instance();
	renderSystem.initialize();
	CameraSystem& cameraSystem = CameraSystem::instance();
	CameraControllerSystem& cameraControllerSystem = CameraControllerSystem::instance();
	PhysicsSystem& physicsSystem = PhysicsSystem::instance();

	renderSystem.setSkybox(&TextureManager::instance().getCubemap({ { "Skybox/right.hdr", "Skybox/left.hdr", "Skybox/bottom.hdr", "Skybox/top.hdr", "Skybox/front.hdr", "Skybox/back.hdr" }, FORMAT_RGBA_HDR16 }));

	// CREATE ENTITIES HERE
	
	const Entity directionalLight;
	directionalLight.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)) });
	directionalLight.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });
	
	// Create camera
	const Entity camera;
	camera.addComponent<Transform>(TransformCreateInfo{glm::vec3(0.0f, 2.5f, 0.0f)});
	camera.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.1f, 100.0f });
	camera.addComponent<CameraController>(CameraController{ 0.0f, 0.005f, true, true, true });
	renderSystem.setCamera(camera);
	
	// --------------------

	std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
	while (!windowManager.windowClosed())
	{
		now = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(now - last).count();

		transformSystem.update();
		renderSystem.update();
		physicsSystem.update(deltaTime);
		cameraSystem.update();
		cameraControllerSystem.update(deltaTime);

		windowManager.pollEvents();

		last = now;
	}

	camera.destroy();
	directionalLight.destroy();
	releaseDeserializedCollections();
	return 0;
}
