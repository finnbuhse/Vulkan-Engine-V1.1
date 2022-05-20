#include "WindowManager.h"
#include "Mesh.h"
#include "Physics.h"
#include "CameraController.h"
#include "SceneManager.h"
#include "FontManager.h"
#include "Gun.h"
#include <chrono>
#include <iostream>

#define ENTITY_PANEL_WIDTH 400
#define ENTITY_PANEL_HEIGHT 700

#define ENTRY_WIDTH 300
#define ENTRY_HEIGHT 40
#define ENTITY_ENTRY_GAP 50

Entity camera("Camera");
Entity entityPanel("Entity panel");

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

void expandEntry(const EntityID& entityID)
{
	Entity entity(entityID);

	unsigned int nComponents = entity.nbComponents();
	unsigned int expansionHeight = nComponents * ENTRY_HEIGHT;

	std::string entityName = entity.name();

	Transform2D& panelTransform = entityPanel.getComponent<Transform2D>();
	EntityID selectedEntry;
	bool found = false;
	bool expand = false;
	for (unsigned int i = 0; i < panelTransform.childrenIDs.length; i++)
	{
		Entity entry(panelTransform.childrenIDs[i]);
		if (!found)
		{
			UIText& entryText = *getComponentsInHierarchy2D<UIText>(entry)[0];
			if (entryText.text == entityName)
			{
				selectedEntry = entry.ID();
				found = true;
				expand = getComponentsInHierarchy2D<UIButton>(entry)[0]->_pressed;
			}
		}
		else
			entry.getComponent<Transform2D>().translate(glm::vec2(0.0f, expand ? float(expansionHeight) : -float(expansionHeight)));
	}

	Entity entry(selectedEntry);
	if (expand)
	{
		unsigned int i = 0;
		Composition composition = entity.composition();
		for (ComponentID componentID = 0; componentID < 64; componentID++)
		{
			if ((composition >> componentID) & 1)
			{
				Entity componentText("Component entry");
				componentText.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(-130.0f, i * 40.0f + 50.0f), 0.0f, glm::vec2(0.5f) });
				entry.getComponent<Transform2D>().addChild(componentText);
				componentText.addComponent<UIText>(UIText{ ComponentManagerBase::componentManagerFromID(componentID).componentName, "Fonts/arial.ttf", glm::vec3(1.0f) });
				i++;
			}
		}
		if (entity.hasComponent<Transform>())
		{
			Transform& transform = entity.getComponent<Transform>();
			for (i = 0; i < transform.childrenIDs.length; i++)
			{

			}
		}
			

	}
	else
	{
		Transform2D* transform = &entry.getComponent<Transform2D>();

		unsigned int i = 0;
		while(i < transform->childrenIDs.length)
		{
			Entity child(transform->childrenIDs[i]);
			if (child.name() == "Component entry")
			{
				destroyChildren(child);
				child.destroy();
			}
			else
				i++;
			transform = &entry.getComponent<Transform2D>();
		}
	}
}

void entityAdded(const Entity& entity)
{
	if (entity.hasComponent<Transform>())
	{
		if (entity.getComponent<Transform>().parentID)
			return;
	}
	else if (entity.hasComponent<Transform2D>())
	{
		if (entity.getComponent<Transform2D>().parentID)
			return;
	}
		
	unsigned int position = entityPanel.getComponent<Transform2D>().childrenIDs.length;

	Entity entityEntry;
	entityEntry.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(-0.1f, position * ENTITY_ENTRY_GAP - 300.0f) });
	entityPanel.getComponent<Transform2D>().addChild(entityEntry);
	entityEntry.addComponent<Sprite>(SpriteCreateInfo{ ENTRY_WIDTH, ENTRY_HEIGHT, "Images/Panel Wide.png" });

	Entity entityName;
	entityName.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(-140.0f, 5.0f), 0.0f, glm::vec2(0.5f)});
	entityEntry.getComponent<Transform2D>().addChild(entityName);
	entityName.addComponent<UIText>(UIText{ entity.name(), "Fonts/arial.ttf", glm::vec3(1.0f) });

	Entity dropdown;
	dropdown.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(130.0f, 0.0f), 0.0f, glm::vec2(1.0f) });
	entityEntry.getComponent<Transform2D>().addChild(dropdown);

	EntityID entityID = entity.ID();
	ButtonCallback callback = [entityID]() { expandEntry(entityID); };
	dropdown.addComponent<UIButton>(UIButtonCreateInfo{ 30, 30, "Images/Dropdown unpressed.png", "Images/Dropdown canpress.png", "Images/Dropdown pressed.png", glm::vec3(1.0f), callback, true });
}


void entityRemoved(const Entity& entity)
{
	static ComponentManager<Transform2D>& transform2DManager = ComponentManager<Transform2D>::instance();
	static ComponentManager<UIText>& UITextManager = ComponentManager<UIText>::instance();

	Transform2D& panelTransform = entityPanel.getComponent<Transform2D>();
	for (unsigned int i = 0; i < panelTransform.childrenIDs.length; i++)
	{
		Entity entry(panelTransform.childrenIDs[i]);
		UIText& entryText = *getComponentsInHierarchy2D<UIText>(entry)[0];
		if (entryText.text == entity.name())
		{
			panelTransform.removeChild(entry);
			destroyChildren(entry);
			entry.destroy();
			break;
		}
	}
}

int main()
{
	WindowManager& windowManager = WindowManager::instance();

	KeyCallback toggleCursorCallback(toggleCursor);
	windowManager.subscribeKeyPressEvent(C, &toggleCursorCallback);

	SceneManager& sceneManager = SceneManager::instance();

	EntityAddedCallback entityAddedCallback(entityAdded);
	sceneManager.subscribeEntityAddedEvent(&entityAddedCallback);

	TransformSystem& transformSystem = TransformSystem::instance();
	RenderSystem& renderSystem = RenderSystem::instance();
	CameraSystem& cameraSystem = CameraSystem::instance();
	CameraControllerSystem& cameraControllerSystem = CameraControllerSystem::instance();
	PhysicsSystem& physicsSystem = PhysicsSystem::instance();
	FontManager& fontManager = FontManager::instance();

	renderSystem.setSkybox(&TextureManager::instance().getCubemap({ { "Images/Skybox/right.hdr", "Images/Skybox/left.hdr", "Images/Skybox/bottom.hdr", "Images/Skybox/top.hdr", "Images/Skybox/front.hdr", "Images/Skybox/back.hdr" }, FORMAT_RGBA_HDR16 }));

	camera.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::quat(0.0f, 0.0f, 0.0f, 1.0f), glm::vec3(1.0f) });
	camera.addComponent<Camera>(CameraCreateInfo{ 90.0f, 1920.0f / 1080.0f, 0.05f, 100.0f });
	camera.addComponent<CameraController>(CameraController{ 5.0f, 0.005f, true, true, true });
	renderSystem.setCamera(camera);

	entityPanel.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(-750.0f, -50.0f) });
	entityPanel.addComponent<Sprite>(SpriteCreateInfo{ ENTITY_PANEL_WIDTH, ENTITY_PANEL_HEIGHT, "Images/Panel Tall.png" });

	// Required for current pipeline
	const Entity directionalLight = sceneManager.createEntity("Light");
	directionalLight.addComponent<Transform>(TransformCreateInfo{ glm::vec3(0.0f), glm::radians(glm::vec3(45.0f, 0.0f, 0.0f)) });
	directionalLight.addComponent<DirectionalLight>(DirectionalLightCreateInfo{ glm::vec3(0.0f) });

	const Entity FPSCounter = sceneManager.createEntity("FPS Counter");
	FPSCounter.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(850.0f, -500.0f), 0.0f, glm::vec2(0.5f) });
	FPSCounter.addComponent<UIText>(UIText{ "", "Fonts/arial.ttf", glm::vec3(1.0f) });

	Entity gun = loadModel("Assets/Floor/Walls.fbx");
	sceneManager.addEntity(gun);

	/*
	Material gunMaterial = MaterialCreateInfo{ "Assets/AK103/AK_103_Base_Color.png", "Assets/AK103/AK_103_Normal.png", "Assets/AK103/AK_103_Roughness.png", "Assets/AK103/AK_103_Metallic.png", "Images/default ambient occlusion.png" };
	applyModelMaterial(gun, gunMaterial);
	Transform& gunTransform = gun.getComponent<Transform>();
	gunTransform.scale = glm::vec3(4.0f);
	gunTransform.position = glm::vec3(0.0f, 5.0f, 0.0f);
	*/

	std::chrono::high_resolution_clock::time_point now, last = std::chrono::high_resolution_clock::now();
	while (!windowManager.windowClosed())
	{
		now = std::chrono::high_resolution_clock::now();
		double deltaTime = std::chrono::duration<double>(now - last).count();

		UIText& FPSText = FPSCounter.getComponent<UIText>();
		FPSText.text = std::to_string((unsigned int)(1.0f / deltaTime)) + " FPS";

		transformSystem.update();
		renderSystem.update();
		cameraSystem.update();
		physicsSystem.update(deltaTime);
		cameraControllerSystem.update(deltaTime);

		windowManager.pollEvents();

		last = now;
	}

	destroyChildren(entityPanel);
	entityPanel.destroy();
	camera.destroy();
	sceneManager.destroyScene();
	return 0;
}