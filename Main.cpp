#include "WindowManager.h"
#include "Mesh.h"
#include "Physics.h"
#include "CameraController.h"
#include "SceneManager.h"
#include "FontManager.h"
#include "InventoryManager.h"
#include "SceneMenu.h"
#include <chrono>
#include <iostream>

int main()
{
	/* INITIALISATION */
	WindowManager& windowManager = WindowManager::instance();
	SceneManager& sceneManager = SceneManager::instance();

	TransformSystem& transformSystem = TransformSystem::instance();
	RenderSystem& renderSystem = RenderSystem::instance();
	CameraSystem& cameraSystem = CameraSystem::instance();
	CameraControllerSystem& cameraControllerSystem = CameraControllerSystem::instance();
	PhysicsSystem& physicsSystem = PhysicsSystem::instance();
	FontManager& fontManager = FontManager::instance();

	InteractSystem& interactSystem = InteractSystem::instance();
	InventoryManager& inventoryManager = InventoryManager::instance();

	#if SCENE_MENU == 1
	SceneMenu& sceneMenu = SceneMenu::instance();
	sceneMenu.setPosition(glm::vec2(windowManager.getWindowWidth() / -2.0f + 175, windowManager.getWindowHeight() / -2.0f + 50));
	windowManager.enableCursor();
	#endif
	/* -------------- */

	renderSystem.setSkybox(&TextureManager::instance().getCubemap({ { "Images/Skybox/right.hdr", "Images/Skybox/left.hdr", "Images/Skybox/bottom.hdr", "Images/Skybox/top.hdr", "Images/Skybox/front.hdr", "Images/Skybox/back.hdr" }, FORMAT_RGBA_HDR16 }));


	Entity light = sceneManager.createEntity("Directional light");
	light.addComponent<Transform>(TransformCreateInfo{});
	light.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });

	
	Entity floor = loadModel("Assets/Cube/Cube.obj");
	sceneManager.addEntity(floor);

	Transform& floorTransform = floor.getComponent<Transform>();
	floorTransform.scale = glm::vec3(40.0f, 1.0f, 40.0f);
	floorTransform.position = glm::vec3(0.0f, -5.0f, 0.0f);

	std::vector<Mesh*> floorMeshes = getComponentsInHierarchy3D<Mesh>(floor);
	floor.addComponent<RigidBody>(StaticRigidBodyCreateInfo{ floorMeshes[0]->positions().data(), floorMeshes[0]->nVertices, floorMeshes[0]->indices, floorMeshes[0]->nIndices, 8, {0.5f, 0.5f, 0.6f} });

	
	const Entity character = sceneManager.createEntity("Player");
	Transform& characterTransform = character.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 10.0f, -5.0f) });
	CharacterControllerCreateInfo controllerCreateInfo = {};
	controllerCreateInfo.radius = 1.0f;
	controllerCreateInfo.height = 5.0f;
	controllerCreateInfo.speed = 15.0f;
	controllerCreateInfo.jumpSpeed = 400.0f;
	controllerCreateInfo.maxStepHeight = 0.2f;
	controllerCreateInfo.maxSlope = 0.7f;
	controllerCreateInfo.invisibleWallHeight = 0.0f;
	controllerCreateInfo.maxJumpHeight = 30.0f;
	controllerCreateInfo.contactOffset = 0.01f;
	controllerCreateInfo.cacheVolumeFactor = 5.0f;
	controllerCreateInfo.slide = true;
	controllerCreateInfo.material = { 0.5f, 0.5f, 0.6f };
	character.addComponent<CharacterController>(controllerCreateInfo);

	const Entity head = sceneManager.createEntity("Head");
	head.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 2.5f, 0.0f), glm::quat(0.0f, 0.0f, 0.0f, 1.0f) });
	character.getComponent<Transform>().addChild(head);

	head.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.1f, 100.0f });
	head.addComponent<CameraController>(CameraController{ 0.0f, 0.005f, false, true, false });
	head.addComponent<Interactor>({ 10.0f });
	renderSystem.setCamera(head);

	
	const Entity weaponItem = loadModel("Assets/AK103/AK_103.fbx");
	applyModelMaterial(weaponItem, MaterialCreateInfo{ "Assets/AK103/AK_103_Base_Color.png", "Assets/AK103/AK_103_Normal.png", "Assets/AK103/AK_103_Roughness.png", "Assets/AK103/AK_103_Metallic.png", "Images/default ambient occlusion.png" });
	sceneManager.addEntity(weaponItem);


	//weaponItem.addComponent<RigidBody>(DynamicRigidBodyCreateInfo{})

	/*
	weaponItem.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 5.0f, 0.0f) });
	weaponItem.addComponent<Interactable>({});
	weaponItem.addComponent<WeaponItem>(WeaponItemCreateInfo{ "AK-47", 3.5f, 5.0f, "AK.txt", glm::vec3(0.0f, 5.0f, 0.0f), glm::vec3(0.0f), glm::vec3(1.0f), 30, 30 });


	*/

	std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
	while (!windowManager.windowClosed())
	{
		now = std::chrono::high_resolution_clock::now();
		double deltaTime = std::chrono::duration<double>(now - last).count();

		transformSystem.update();
		renderSystem.update();
		cameraSystem.update();
		physicsSystem.update(deltaTime);
		cameraControllerSystem.update(deltaTime);

		windowManager.pollEvents();

		last = now;
	}
	sceneMenu.destroyMenu();
	sceneManager.destroyScene();
	return 0;
}