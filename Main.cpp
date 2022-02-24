#include "WindowManager.h"
#include "Mesh.h"
#include "CameraController.h"
#include "Gun.h"
#include "SceneManager.h"
#include "FontManager.h"
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
	InteractSystem& interactSystem = InteractSystem::instance();
	GunSystem& gunSystem = GunSystem::instance();
	ComponentManager<Interactable>& interactableManager = ComponentManager<Interactable>::instance();
	ComponentManager<GunItem>& gunItemManager = ComponentManager<GunItem>::instance();
	ComponentManager<GunWeapon>& gunWeaponManager = ComponentManager<GunWeapon>::instance();
	SceneManager& sceneManager = SceneManager::instance();
	FontManager& fontManager = FontManager::instance();

	renderSystem.setSkybox(&TextureManager::instance().getCubemap({ { "Images/Skybox/right.hdr", "Images/Skybox/left.hdr", "Images/Skybox/bottom.hdr", "Images/Skybox/top.hdr", "Images/Skybox/front.hdr", "Images/Skybox/back.hdr" }, FORMAT_RGBA_HDR16 }));

	const Entity FPSCounter = sceneManager.createEntity();
	UIText& FPSText = FPSCounter.addComponent<UIText>(UIText{ glm::vec2(0.875f, -0.9f), 1.0f, "", "Fonts/arial.ttf", glm::vec3(1.0f)});

	const Entity directionalLight = sceneManager.createEntity();
	directionalLight.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)) });
	directionalLight.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });
	
	// Bullet
	Entity bullet = loadModel("Assets/AK103/Bullet.fbx");
	std::vector<Mesh*> bulletMeshes = getComponentsInHierarchy<Mesh>(bullet);

	Transform& bulletTransform = bullet.getComponent<Transform>();
	bulletTransform.position = glm::vec3(5.0f, 5.0f, 0.0f);
	bulletTransform.scale = glm::vec3(4.0f);
	ComponentManager<Transform>::instance().getComponent(bulletTransform.childrenIDs[0]).rotate(PI/2.0f, glm::vec3(0.0f, 1.0f, 0.0f));

	bullet.addComponent<RigidBody>(DynamicRigidBodyCreateInfo{ bulletMeshes[0]->positions().data(), bulletMeshes[0]->nVertices, 30, {0.5f, 0.5f, 0.6f}, 1.0f });
	bullet.addComponent<Bullet>(Bullet{ glm::vec3(0.0f, 0.0f, -1.0f) });
	writeFile("Bullet.txt", serialize(bullet));

	destroyChildren(bullet);
	bullet.destroy();

	// AK 47
	Entity gun = loadModel("Assets/AK103/AK_103.fbx");
	sceneManager.addEntity(gun);

	Material gunMaterial = MaterialCreateInfo{ "Assets/AK103/AK_103_Base_Color.png", "Assets/AK103/AK_103_Normal.png", "Assets/AK103/AK_103_Roughness.png", "Assets/AK103/AK_103_Metallic.png", "Images/default ambient occlusion.png" };
	applyModelMaterial(gun, gunMaterial);
	Transform& gunTransform = gun.getComponent<Transform>();
	gunTransform.scale = glm::vec3(4.0f);
	gunTransform.position = glm::vec3(0.0f, 5.0f, 0.0f);

	GunWeapon& gunWeapon = gun.addComponent<GunWeapon>(GunWeapon{ glm::vec3(0.5f, -1.1f, -1.3f), glm::radians(glm::vec3(0.0f, 90.0f, 0.0f)), glm::vec3(4.0f), glm::vec3(0.0f, 30.0f, -50.0f), glm::vec3(0.08f, 0.75f, -1.75f) });
	gunWeapon.initialize("GunItem.txt", "Bullet.txt");
	writeFile("GunWeapon.txt", serialize(gun));
	gun.removeComponent<GunWeapon>();

	std::vector<Mesh*> gunMeshes = getComponentsInHierarchy<Mesh>(gun);
	gun.addComponent<RigidBody>(DynamicRigidBodyCreateInfo{ gunMeshes[4]->positions().data(), gunMeshes[4]->nVertices, 25, {0.7f, 0.7f, 0.6f}, 1.0f });
	gun.addComponent<Interactable>({});
	GunItem& gunItem = gun.addComponent<GunItem>({ glm::vec3(0.5f, -1.1f, -1.3f), glm::radians(glm::vec3(0.0f, 90.0f, 0.0f)), glm::vec3(4.0f) });
	gunItem.initialize("GunWeapon.txt");
	writeFile("GunItem.txt", serialize(gun));
	// ----- 
	
	Entity floor = loadModel("Assets/Cube/Cube.obj");
	sceneManager.addEntity(floor);

	Transform& floorTransform = floor.getComponent<Transform>();
	floorTransform.scale = glm::vec3(40.0f, 1.0f, 40.0f);
	floorTransform.position = glm::vec3(0.0f, -5.0f, 0.0f);

	std::vector<Mesh*> floorMeshes = getComponentsInHierarchy<Mesh>(floor);
	floor.addComponent<RigidBody>(StaticRigidBodyCreateInfo{ floorMeshes[0]->positions().data(), floorMeshes[0]->nVertices, floorMeshes[0]->indices, floorMeshes[0]->nIndices, 8, {0.5f, 0.5f, 0.6f} });

	const Entity character = sceneManager.createEntity();
	Transform& characterTransform = character.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f, 3.0f, -5.0f) });
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

	const Entity head = sceneManager.createEntity();
	head.addComponent<Transform>(TransformCreateInfo{glm::vec3(0.0f, 2.5f, 0.0f)});
	characterTransform.addChild(head);

	head.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.1f, 100.0f });
	head.addComponent<CameraController>(CameraController{ 0.0f, 0.005f, false, true, false });
	head.addComponent<Interactor>({ 10.0f });

	renderSystem.setCamera(head);

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