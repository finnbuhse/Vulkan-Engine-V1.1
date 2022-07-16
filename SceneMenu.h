#pragma once
#include "SceneManager.h"
#include "Mesh.h"

struct EntityButtonInfo
{
	EntityID sceneEntity;
};

class SceneMenu
{
private:
	SceneManager& mSceneManager = SceneManager::instance();

	const EntityAddedCallback mEntityAddedCallback = std::bind(&SceneMenu::addEntityButton, this, std::placeholders::_1);
	const EntityRemovedCallback mEntityRemovedCallback = std::bind(&SceneMenu::removeEntityButton, this, std::placeholders::_1);
	const ButtonCallback mEntityButtonCallback = std::bind(&SceneMenu::entityButtonPressed, this, std::placeholders::_1);

	Entity mSceneMenu;

	glm::vec2 mNextButtonPosition = glm::vec2(0, 0);

	glm::ivec2 mButtonDimensions = glm::vec2(200, 20);
	glm::vec2 mButtonTextOffset = glm::vec2(10.0f, 5.0f);
	glm::vec2 mButtonTextScale = glm::vec2(0.3f, 0.3f);

	std::string mFont = "Fonts/arial.ttf";
	
	SceneMenu();

public:
	~SceneMenu();

	static SceneMenu& instance();

	void destroyMenu();

	void addEntityButton(const Entity& entity);
	void removeEntityButton(const Entity& entity);

	void setPosition(const glm::vec2& position);
	void setFont(const std::string& font);
	void setButtonTextOffset(const glm::vec2 &offset);

	void entityButtonPressed(const Entity& button);
};