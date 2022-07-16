#include "SceneMenu.h"

SceneMenu::SceneMenu()
{
	mSceneManager.subscribeEntityAddedEvent(&mEntityAddedCallback);
	mSceneManager.subscribeEntityRemovedEvent(&mEntityRemovedCallback);

	mSceneMenu.addComponent<Transform2D>(Transform2DCreateInfo{});
}

SceneMenu::~SceneMenu(){}

SceneMenu& SceneMenu::instance()
{
	static SceneMenu instance;
	return instance;
}

void SceneMenu::destroyMenu()
{
	mSceneManager.unsubscribeEntityAddedEvent(&mEntityAddedCallback);
	mSceneManager.unsubscribeEntityRemovedEvent(&mEntityRemovedCallback);

	destroyChildren(mSceneMenu);
	mSceneMenu.destroy();
}

void SceneMenu::addEntityButton(const Entity& entity)
{
	if (entity.hasComponent<Transform>())
	{
		if (entity.getComponent<Transform>().parentID != NULL)
			return;
	}
	if (entity.hasComponent<Transform2D>())
	{
		if (entity.getComponent<Transform2D>().parentID != NULL)
			return;
	}
	const Entity entityButton("Entity Menu Button");
	entityButton.addComponent<Transform2D>(Transform2DCreateInfo{mNextButtonPosition});
	entityButton.addComponent<UIButton>(UIButtonCreateInfo{ (unsigned int)mButtonDimensions.x, (unsigned int)mButtonDimensions.y, "Images/Entity Button Unpressed.png", "Images/Entity Button Canpress.png", "Images/Entity Button Pressed.png", glm::vec3(1.0f), mEntityButtonCallback, true });
	entityButton.addComponent<EntityButtonInfo>({ entity.ID() });

	const Entity entityButtonText("Entity Button Text");
	entityButtonText.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(-mButtonDimensions.x/2, 0.0f) + mButtonTextOffset, 0, mButtonTextScale });
	entityButtonText.addComponent<UIText>(UIText{ entity.name(), mFont, glm::vec3(1.0f, 1.0f, 1.0f) });
	entityButton.getComponent<Transform2D>().addChild(entityButtonText);

	mNextButtonPosition.y += mButtonDimensions.y;
	mSceneMenu.getComponent<Transform2D>().addChild(entityButton);
}

void SceneMenu::removeEntityButton(const Entity& entity)
{
	bool found = false;
	unsigned int i = 0;
	Transform2D& menuTransform = mSceneMenu.getComponent<Transform2D>();
	while (i < menuTransform.childrenIDs.length)
	{
		Entity button(menuTransform.childrenIDs[i]);
		if (found)
		{
			button.getComponent<Transform2D>().translate(glm::vec2(0.0f, -mButtonDimensions.y));
			i++;
		}
		else if (button.getComponent<EntityButtonInfo>().sceneEntity == entity.ID())
		{
			destroyChildren(button);
			button.destroy();
			mNextButtonPosition.y -= mButtonDimensions.y;
			found = true;
		}
		else
			i++;
	}
}

void SceneMenu::setPosition(const glm::vec2& position)
{
	mSceneMenu.getComponent<Transform2D>().position = position;
}

void SceneMenu::setFont(const std::string& font)
{
	mFont = font;
}

void SceneMenu::setButtonTextOffset(const glm::vec2& offset)
{
	mButtonTextOffset = offset;
}

void SceneMenu::entityButtonPressed(const Entity& button)
{
	Entity sceneEntity(button.getComponent<EntityButtonInfo>().sceneEntity);
}
