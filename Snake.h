#pragma once
#include "Physics.h"
#include "Mesh.h"

#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define SQUARE_SIZE 2.0f

struct Turn
{
	glm::vec2 position;
	glm::vec2 direction;
};

struct Snake
{
	float movementSpeed;

	glm::vec2 _velocity;
	glm::vec2 _queuedDirection;
	Vector<Turn> _turns;
};

struct SnakeCreateInfo
{
	float movementSpeed;

	operator Snake()
	{
		Snake snake;
		snake.movementSpeed = movementSpeed;
		snake._velocity = glm::vec2(0.0f, -movementSpeed);
		snake._queuedDirection = glm::vec2(0.0f);
		return snake;
	}
};

class SnakeSystem
{
private:
	SnakeSystem();

	WindowManager& mWindowManager = WindowManager::instance();
	ComponentManager<Transform>& mTransformManager = ComponentManager<Transform>::instance();
	ComponentManager<Snake>& mSnakeManager = ComponentManager<Snake>::instance();
	ComponentManager<RigidBody>& mRigidBodyManager = ComponentManager<RigidBody>::instance();
	ComponentManager<Transform2D>& mTransform2DManager = ComponentManager<Transform2D>::instance();
	ComponentManager<Sprite>& mSpriteManager = ComponentManager<Sprite>::instance();
	ComponentManager<UIText>& mUITextManager = ComponentManager<UIText>::instance();
	
	const ButtonCallback mStartCallback = std::bind(&SnakeSystem::startGame, this);

	const KeyCallback upCallback = std::bind(&SnakeSystem::upPressed, this);
	const KeyCallback downCallback = std::bind(&SnakeSystem::downPressed, this);
	const KeyCallback leftCallback = std::bind(&SnakeSystem::leftPressed, this);
	const KeyCallback rightCallback = std::bind(&SnakeSystem::rightPressed, this);

	std::vector<char> mSerializedSegment;

	EntityID mSnake = 0;
	EntityID mMenu = 0;
	EntityID mLengthText = 0;

	void destroySnake();
	void destroyMenu();

public:
	static SnakeSystem& instance();

	~SnakeSystem();

	void startGame();
	void menu();

	void update(const float& deltaTime);
	void end();

	void upPressed();
	void downPressed();
	void leftPressed();
	void rightPressed();
};