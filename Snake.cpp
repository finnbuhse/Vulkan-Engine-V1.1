#include "Snake.h"
#include "SceneManager.h"
#include "Math.h"

SnakeSystem::SnakeSystem()
{
	mSerializedSegment = readFile("Segment.txt");
}

void SnakeSystem::destroySnake()
{
	mWindowManager.unsubscribeKeyPressEvent(W, &upCallback);
	mWindowManager.unsubscribeKeyPressEvent(S, &downCallback);
	mWindowManager.unsubscribeKeyPressEvent(A, &leftCallback);
	mWindowManager.unsubscribeKeyPressEvent(D, &rightCallback);

	Snake& snake = mSnakeManager.getComponent(mSnake);
	snake._turns.free();

	Entity snakeEntity(mSnake);
	destroyChildren(snakeEntity);
	snakeEntity.destroy();
	mSnake = 0;

	Entity(mLengthText).destroy();
	mLengthText = 0;
}

void SnakeSystem::destroyMenu()
{
	Entity menuEntity(mMenu);
	destroyChildren(menuEntity);
	menuEntity.destroy();
	mMenu = 0;
}

SnakeSystem& SnakeSystem::instance()
{
	static SnakeSystem instance;
	return instance;
}

SnakeSystem::~SnakeSystem()
{
}

void SnakeSystem::startGame()
{
	mWindowManager.disableCursor();

	destroyMenu();

	mWindowManager.subscribeKeyPressEvent(W, &upCallback);
	mWindowManager.subscribeKeyPressEvent(S, &downCallback);
	mWindowManager.subscribeKeyPressEvent(A, &leftCallback);
	mWindowManager.subscribeKeyPressEvent(D, &rightCallback);

	mSnake = Entity("Snake").ID();
	mTransformManager.addComponent(mSnake, TransformCreateInfo{});
	Snake& snake = mSnakeManager.addComponent(mSnake, SnakeCreateInfo{ 15.0f });
	snake._turns.initialize();

	mLengthText = Entity("Length UI Text").ID();
	mTransform2DManager.addComponent(mLengthText, Transform2DCreateInfo{glm::vec2(-950.0f, -500.0f), 0.0f, glm::vec2(0.5f)});
	mUITextManager.addComponent(mLengthText, UIText{ "", "Fonts/arial.ttf", glm::vec3(1.0f) });

	Entity head("Head");
	deserialize(readFile("Head.txt"), head);
	Transform& transform = mTransformManager.getComponent(mSnake);
	transform.addChild(head);
}

void SnakeSystem::menu()
{
	mWindowManager.enableCursor();

	if(mSnake)
		destroySnake();

	mMenu = Entity("Menu").ID();
	mTransform2DManager.addComponent(mMenu, Transform2DCreateInfo{ glm::vec2(0.0f), 0.0f, glm::vec2(mWindowManager.getWindowWidth(), mWindowManager.getWindowHeight()) });
	mSpriteManager.addComponent(mMenu, SpriteCreateInfo{"Images/Menu Background.png"});

	Entity playButton("Play Button");
	playButton.addComponent<Transform2D>(Transform2DCreateInfo{ glm::vec2(0.0f, 0.1f), 0.0f, glm::vec2(0.2f, 0.2f) });
	playButton.addComponent<UIButton>(UIButtonCreateInfo{"Images/Play Unpressed.png", "Images/Play Canpress.png", "Images/Play Pressed.png", glm::vec3(1.0f), mStartCallback});
	Transform2D& transform = mTransform2DManager.getComponent(mMenu);
	transform.addChild(playButton);
}


void SnakeSystem::update(const float& deltaTime)
{
	if (mSnake)
	{
		Transform* transform = &mTransformManager.getComponent(mSnake);
		Snake& snake = mSnakeManager.getComponent(mSnake);

		Transform& headTransform = mTransformManager.getComponent(transform->childrenIDs[0]);

		mTransformManager.getComponent(transform->childrenIDs[0]).translate(glm::vec3(snake._velocity.x, 0.0f, snake._velocity.y) * deltaTime);

		glm::vec2 headDirection = glm::normalize(snake._velocity);
		glm::vec2 tailDirection = -headDirection;

		PhysicsSystem& physicsSystem = PhysicsSystem::instance();
		physx::PxRaycastBuffer ray = physicsSystem.raycast(headTransform.position, glm::vec3(headDirection.x, 0.0f, headDirection.y), SQUARE_SIZE / 2.0f, UNDEFINED);

		if (ray.hasBlock)
		{
			for (unsigned int i = 0; i < ray.getNbAnyHits(); i++)
			{
				const physx::PxRaycastHit hit = ray.getAnyHit(i);
				Entity hitEntity(*((unsigned int*)hit.actor->userData));
				std::string name = hitEntity.name();
				if (name == "Food")
				{
					hitEntity.getComponent<Transform>().position = glm::vec3(float(rand() % GRID_WIDTH - (GRID_WIDTH / 2.0f)) * SQUARE_SIZE, 0.0f, float(rand() % GRID_HEIGHT - (GRID_HEIGHT / 2.0f)) * SQUARE_SIZE);

					Entity segment("Segment");
					deserialize(mSerializedSegment, segment);

					transform = &mTransformManager.getComponent(mSnake);
					segment.getComponent<Transform>().position = mTransformManager.getComponent(transform->childrenIDs[transform->childrenIDs.length - 1]).position;
					transform->addChild(segment);
				}

				if (name == "Segment" || name == "Walls")
				{
					menu();
					return;
				}
			}
		}

		glm::vec2 nextGrid;
		if (snake._queuedDirection != glm::vec2(0.0f, 0.0f))
		{
			if (snake._velocity.x > 0.0f)
				nextGrid = glm::vec2(roundUpToMultiple(headTransform.position.x, SQUARE_SIZE), headTransform.position.z);
			else if (snake._velocity.x < 0.0f)
				nextGrid = glm::vec2(roundDownToMultiple(headTransform.position.x, SQUARE_SIZE), headTransform.position.z);
			else if (snake._velocity.y > 0.0f)
				nextGrid = glm::vec2(headTransform.position.x, roundUpToMultiple(headTransform.position.z, SQUARE_SIZE));
			else if (snake._velocity.y < 0.0f)
				nextGrid = glm::vec2(headTransform.position.x, roundDownToMultiple(headTransform.position.z, SQUARE_SIZE));

			if (abs(nextGrid.x - headTransform.position.x) < 0.2f && abs(nextGrid.y - headTransform.position.z) < 0.2f)
			{
				headTransform.position = glm::vec3(nextGrid.x, 0.0f, nextGrid.y);

				snake._velocity = snake._queuedDirection * snake.movementSpeed;
				snake._queuedDirection = glm::vec2(0.0f, 0.0f);

				if (transform->childrenIDs.length > 1)
					snake._turns.push({ nextGrid, tailDirection });
			}
		}

		glm::vec2 segmentPosition = glm::vec2(headTransform.position.x, headTransform.position.z);
		int turnIndex = snake._turns.length - 1;
		for (unsigned int i = 1; i < transform->childrenIDs.length; i++)
		{
			segmentPosition += tailDirection * SQUARE_SIZE;
			if (turnIndex >= 0)
			{
				glm::vec2 segmentTurn = snake._turns[turnIndex].position - segmentPosition;
				if ((tailDirection.x > 0.0f && segmentTurn.x < 0.0f) || (tailDirection.x < 0.0f && segmentTurn.x > 0.0f) || (tailDirection.y > 0.0f && segmentTurn.y < 0.0f) || (tailDirection.y < 0.0f && segmentTurn.y > 0.0f))
				{
					segmentPosition = snake._turns[turnIndex].position + snake._turns[turnIndex].direction * glm::length(segmentTurn);
					tailDirection = snake._turns[turnIndex].direction;
					turnIndex--;
				}
			}

			mRigidBodyManager.getComponent(transform->childrenIDs[i]).setTransform(glm::vec3(segmentPosition.x, 0.0f, segmentPosition.y));
		}
		if (turnIndex >= 0)
			snake._turns.remove(0);

		UIText& lengthText = mUITextManager.getComponent(mLengthText);
		lengthText.text = "Length: " + std::to_string(transform->childrenIDs.length);
	}
}

void SnakeSystem::end()
{
	if (mSnake)
		destroySnake();
	if (mMenu)
		destroyMenu();
}

void SnakeSystem::upPressed()
{
	if (mSnake)
	{
		Snake& snake = mSnakeManager.getComponent(mSnake);
		if (snake._velocity.y == 0.0f)
			snake._queuedDirection = glm::vec2(0.0f, -1.0f);
	}
}

void SnakeSystem::downPressed()
{
	if (mSnake)
	{
		Snake& snake = mSnakeManager.getComponent(mSnake);
		if (snake._velocity.y == 0.0f)
			snake._queuedDirection = glm::vec2(0.0f, 1.0f);
	}
}

void SnakeSystem::leftPressed()
{
	if (mSnake)
	{
		Snake& snake = mSnakeManager.getComponent(mSnake);
		if (snake._velocity.x == 0.0f)
			snake._queuedDirection = glm::vec2(-1.0f, 0.0f);
	}
}

void SnakeSystem::rightPressed()
{
	if (mSnake)
	{
		Snake& snake = mSnakeManager.getComponent(mSnake);
		if (snake._velocity.x == 0.0f)
			snake._queuedDirection = glm::vec2(1.0f, 0.0f);
	}
}
