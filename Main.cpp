#include "WindowManager.h"
#include "Mesh.h"
#include "CameraController.h"
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

	renderSystem.setSkybox(&TextureManager::instance().getCubemap({ { "Skybox/right.hdr", "Skybox/left.hdr", "Skybox/top.hdr", "Skybox/bottom.hdr", "Skybox/front.hdr", "Skybox/back.hdr" }, VK_FORMAT_R8G8B8A8_UNORM/*VK_FORMAT_R32G32B32A32_SFLOAT*/ }));

	// Create camera
	const Entity camera;
	camera.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 0.0f, 10.0f) });
	camera.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.1f, 100.0f });
	camera.addComponent<CameraController>(CameraController{ 10.0f, 0.005f });
	renderSystem.setCamera(camera);

	const Entity directionalLight;
	directionalLight.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)) });
	directionalLight.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });

	std::vector<Entity> gun = loadModel("AK103/AK_103.fbx");
	
	ComponentBit meshBit = ComponentManager<Mesh>::instance().bit;
	Material gunMaterial = MaterialCreateInfo{ "AK103/AK_103_Base_Color.png", "AK103/AK_103_Normal.png", "AK103/AK_103_Roughness.png", "AK103/AK_103_Metallic.png", "default ambient occlusion.png" };
	for (const Entity& entity : gun)
	{
		if (entity.composition() & meshBit)
		{
			Mesh& mesh = entity.getComponent<Mesh>();
			mesh.material = gunMaterial;
			mesh.updateMaterial();
		}
	}

	Transform& gunTransform = gun.back().getComponent<Transform>();
	gunTransform.rotation = glm::radians(glm::vec3(180.0f, 0.0f, 0.0f));
	gunTransform.scale = glm::vec3(0.1f);

	std::vector<Entity> floor = loadModel("Cube/Cube.obj");
	Transform& floorTransform = floor.back().getComponent<Transform>();
	floorTransform.scale = glm::vec3(40.0f, 1.0f, 40.0f);
	floorTransform.position = glm::vec3(0.0f, 3.0f, 0.0f);

	std::chrono::high_resolution_clock::time_point now, last;
	while (!windowManager.windowClosed())
	{
		now = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float>(now - last).count();

		transformSystem.update();
		cameraSystem.update();
		cameraControllerSystem.update(deltaTime);
		renderSystem.update();

		gun.back().getComponent<Transform>().rotate(glm::radians(0.5f), glm::vec3(0.0f, 1.0f, 0.0f));

		windowManager.pollEvents();

		last = now;
	}

	camera.destroy();
	directionalLight.destroy();
	destroyModel(gun);
	destroyModel(floor);
	return 0;
}