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

	const Entity directionalLight;
	directionalLight.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)) });
	directionalLight.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });

	std::vector<Entity> gun = loadModel("AK103/AK_103.fbx");
	Material gunMaterial = MaterialCreateInfo{ "AK103/AK_103_Base_Color.png", "AK103/AK_103_Normal.png", "AK103/AK_103_Roughness.png", "AK103/AK_103_Metallic.png", "default ambient occlusion.png" };
	applyMaterial(gun, gunMaterial);

	std::vector<Entity> floor = loadModel("Cube/Cube.obj");
	Transform& floorTransform = floor.back().getComponent<Transform>();
	floorTransform.scale = glm::vec3(40.0f, 1.0f, 40.0f);
	floorTransform.position = glm::vec3(0.0f, -5.0f, 0.0f);

	std::vector<Mesh*> floorMeshes = findComponentsInModel<Mesh>(floor);
	floor.back().addComponent<RigidBody>(StaticRigidBodyCreateInfo{ floorMeshes[0], 8, {0.5f, 0.5f, 0.6f} });

	std::vector<char> floorBytes = serialize(*(floorMeshes[0]));
	const Entity floor2;
	floor2.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 10.0f, 0.0f) });
	Mesh& floorMesh2 = floor2.addComponent<Mesh>({ 0, 0, MaterialCreateInfo{} });
	deserialize(floorBytes, floorMesh2);

	Transform& gunTransform = gun.back().getComponent<Transform>();
	gunTransform.position = glm::vec3(0.0f, 10.0f, 0.0f);
	gunTransform.rotate(glm::radians(30.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	gunTransform.scale = glm::vec3(4.0f);

	std::vector<Mesh*> gunMeshes = findComponentsInModel<Mesh>(gun);
	gun.back().addComponent<RigidBody>(DynamicRigidBodyCreateInfo{ gunMeshes[0], 40, {0.5f, 0.5f, 0.6f}, 1.0f, false });

	const Entity character;
	Transform& characterTransform = character.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 3.0f, -5.0f) });
	CharacterControllerCreateInfo controllerCreateInfo = {};
	controllerCreateInfo.radius = 1.0f;
	controllerCreateInfo.height = 4.0f;
	controllerCreateInfo.speed = 15.0f;
	controllerCreateInfo.jumpSpeed = 7.0f;
	controllerCreateInfo.maxStepHeight = 0.2f;
	controllerCreateInfo.maxSlope = 0.7f;
	controllerCreateInfo.invisibleWallHeight = 0.0f;
	controllerCreateInfo.maxJumpHeight = 30.0f;
	controllerCreateInfo.contactOffset = 0.01f;
	controllerCreateInfo.cacheVolumeFactor = 1.5f;
	controllerCreateInfo.slide = true;
	controllerCreateInfo.material = { 0.5f, 0.5f, 0.6f };
	character.addComponent<CharacterController>(controllerCreateInfo);

	// Create camera
	const Entity camera;
	camera.addComponent<Transform>(TransformCreateInfo{glm::vec3(0.0f, 2.5f, 0.0f)});
	camera.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.1f, 100.0f });
	characterTransform.addChild(camera);
	camera.addComponent<CameraController>(CameraController{ 0.0f, 0.005f, false, true, false });
	renderSystem.setCamera(camera);

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

	floor2.destroy();
	camera.destroy();
	directionalLight.destroy();
	destroyModel(gun);
	destroyModel(floor);
	return 0;
}