#include "WindowManager.h"
#include <cassert>

Key translateGLFWKey(const int& key)
{
	switch (key)
	{
	case (GLFW_KEY_A):
		return Key::A;
	case (GLFW_KEY_B):
		return Key::B;
	case (GLFW_KEY_C):
		return Key::C;
	case (GLFW_KEY_D):
		return Key::D;
	case (GLFW_KEY_E):
		return Key::E;
	case (GLFW_KEY_F):
		return Key::F;
	case (GLFW_KEY_G):
		return Key::G;
	case (GLFW_KEY_H):
		return Key::H;
	case (GLFW_KEY_I):
		return Key::I;
	case (GLFW_KEY_J):
		return Key::J;
	case (GLFW_KEY_K):
		return Key::K;
	case (GLFW_KEY_L):
		return Key::L;
	case (GLFW_KEY_M):
		return Key::M;
	case (GLFW_KEY_N):
		return Key::N;
	case (GLFW_KEY_O):
		return Key::O;
	case (GLFW_KEY_P):
		return Key::P;
	case (GLFW_KEY_Q):
		return Key::Q;
	case (GLFW_KEY_R):
		return Key::R;
	case (GLFW_KEY_S):
		return Key::S;
	case (GLFW_KEY_T):
		return Key::T;
	case (GLFW_KEY_U):
		return Key::U;
	case (GLFW_KEY_V):
		return Key::V;
	case (GLFW_KEY_W):
		return Key::W;
	case (GLFW_KEY_X):
		return Key::X;
	case (GLFW_KEY_Y):
		return Key::Y;
	case (GLFW_KEY_Z):
		return Key::Z;
	case (GLFW_KEY_0):
		return Key::Numeric0;
	case (GLFW_KEY_1):
		return Key::Numeric1;
	case (GLFW_KEY_2):
		return Key::Numeric2;
	case (GLFW_KEY_3):
		return Key::Numeric3;
	case (GLFW_KEY_4):
		return Key::Numeric4;
	case (GLFW_KEY_5):
		return Key::Numeric5;
	case (GLFW_KEY_6):
		return Key::Numeric6;
	case (GLFW_KEY_7):
		return Key::Numeric7;
	case (GLFW_KEY_8):
		return Key::Numeric8;
	case (GLFW_KEY_9):
		return Key::Numeric9;
	case (GLFW_KEY_SPACE):
		return Key::Space;
	case (GLFW_KEY_LEFT_BRACKET):
		return Key::LeftBracket;
	case (GLFW_KEY_RIGHT_BRACKET):
		return Key::RightBracket;
	case (GLFW_KEY_SEMICOLON):
		return Key::Semicolon;
	case (GLFW_KEY_APOSTROPHE):
		return Key::Apostrophe;
	case (GLFW_KEY_BACKSLASH):
		return Key::Backslash;
	case (GLFW_KEY_SLASH):
		return Key::Slash;
	case (GLFW_KEY_COMMA):
		return Key::Comma;
	case (GLFW_KEY_PERIOD):
		return Key::Dot;
	default:
		return Key::UNKNOWN;
	}
}

int translateKey(const Key& key)
{
	switch (key)
	{
	case (Key::A):
		return GLFW_KEY_A;
	case (Key::B):
		return GLFW_KEY_B;
	case (Key::C):
		return GLFW_KEY_C;
	case (Key::D):
		return GLFW_KEY_D;
	case (Key::E):
		return GLFW_KEY_E;
	case (Key::F):
		return GLFW_KEY_F;
	case (Key::G):
		return GLFW_KEY_G;
	case (Key::H):
		return GLFW_KEY_H;
	case (Key::I):
		return GLFW_KEY_I;
	case (Key::J):
		return GLFW_KEY_J;
	case (Key::K):
		return GLFW_KEY_K;
	case (Key::L):
		return GLFW_KEY_L;
	case (Key::M):
		return GLFW_KEY_M;
	case (Key::N):
		return GLFW_KEY_N;
	case (Key::O):
		return GLFW_KEY_O;
	case (Key::P):
		return GLFW_KEY_P;
	case (Key::Q):
		return GLFW_KEY_Q;
	case (Key::R):
		return GLFW_KEY_R;
	case (Key::S):
		return GLFW_KEY_S;
	case (Key::T):
		return GLFW_KEY_T;
	case (Key::U):
		return GLFW_KEY_U;
	case (Key::V):
		return GLFW_KEY_V;
	case (Key::W):
		return GLFW_KEY_W;
	case (Key::X):
		return GLFW_KEY_X;
	case (Key::Y):
		return GLFW_KEY_Y;
	case (Key::Z):
		return GLFW_KEY_Z;
	case (Key::Numeric0):
		return GLFW_KEY_0;
	case (Key::Numeric1):
		return GLFW_KEY_1;
	case (Key::Numeric2):
		return GLFW_KEY_2;
	case (Key::Numeric3):
		return GLFW_KEY_3;
	case (Key::Numeric4):
		return GLFW_KEY_4;
	case (Key::Numeric5):
		return GLFW_KEY_5;
	case (Key::Numeric6):
		return GLFW_KEY_6;
	case (Key::Numeric7):
		return GLFW_KEY_7;
	case (Key::Numeric8):
		return GLFW_KEY_8;
	case (Key::Numeric9):
		return GLFW_KEY_9;
	case (Key::Space):
		return GLFW_KEY_SPACE;
	case (Key::LeftBracket):
		return GLFW_KEY_LEFT_BRACKET;
	case (Key::RightBracket):
		return GLFW_KEY_RIGHT_BRACKET;
	case (Key::Semicolon):
		return GLFW_KEY_SEMICOLON;
	case (Key::Apostrophe):
		return GLFW_KEY_APOSTROPHE;
	case (Key::Backslash):
		return GLFW_KEY_BACKSLASH;
	case (Key::Slash):
		return GLFW_KEY_SLASH;
	case (Key::Comma):
		return GLFW_KEY_COMMA;
	case (Key::Dot):
		return GLFW_KEY_PERIOD;
	default:
		return GLFW_KEY_UNKNOWN;
	}
}

void keyCallback(GLFWwindow* window, int GLFWKey, int scancode, int action, int mods)
{
	static WindowManager& windowManager = WindowManager::instance();

	Key key = translateGLFWKey(GLFWKey);
	if (key == UNKNOWN)
		return;
	switch (action)
	{
	case (GLFW_PRESS):
		for (KeyCallback* callback : windowManager.mKeyPressCallbacks[key])
			(*callback)();
		break;
	case (GLFW_RELEASE):
		for (KeyCallback* callback : windowManager.mKeyReleaseCallbacks[key])
			(*callback)();
		break;
	}
}

#include <iostream>

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	static WindowManager& windowManager = WindowManager::instance();

	switch (action)
	{
	case GLFW_PRESS:
		switch (button)
		{
		case 0:
			for (MouseButtonCallback* callback : windowManager.mLMBPressedCallbacks)
				(*callback)();
			break;
		case 1:
			for (MouseButtonCallback* callback : windowManager.mRMBPressedCallbacks)
				(*callback)();
			break;
		default:
			break;
		}
		break;
	case GLFW_RELEASE:
		switch (button)
		{
		case 0:
			for (MouseButtonCallback* callback : windowManager.mLMBReleasedCallbacks)
				(*callback)();
			break;
		case 1:
			for (MouseButtonCallback* callback : windowManager.mRMBReleasedCallbacks)
				(*callback)();
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

void cursorMovedCallback(GLFWwindow* window, double x, double y)
{
	static WindowManager& windowManager = WindowManager::instance();
	static glm::vec2 lastPosition;

	glm::vec2 position = glm::vec2(x, y);
	glm::vec2 delta = position - lastPosition;
	lastPosition = position;

	for (CursorMovedCallback* callback : windowManager.mCursorMovedCallbacks)
		(*callback)(position, delta);
}

WindowManager::WindowManager(const char* windowTitle, const unsigned int& windowWidth, const unsigned int& windowHeight) :
	mWindowTitle(windowTitle), mWindowWidth(windowWidth), mWindowHeight(windowHeight)
{
	bool result = glfwInit();
	assert(("[ERROR] GLFW initialization failed", result));

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	mWindow = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
	assert(("[ERROR] Window creation failed", mWindow != nullptr));

	glfwSetCursorPos(mWindow, mWindowWidth / 2, mWindowHeight / 2);
	
    if(DEFAULT_DISABLE_CURSOR)
		glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetKeyCallback(mWindow, keyCallback);
	glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
	glfwSetCursorPosCallback(mWindow, cursorMovedCallback);
}

WindowManager& WindowManager::instance()
{
	static WindowManager instance(WINDOW_TITLE, 1920, 1080);
	return instance;
}

WindowManager::~WindowManager()
{
	glfwTerminate();
}

bool WindowManager::windowClosed() const
{
	return glfwWindowShouldClose(mWindow);
}

void WindowManager::pollEvents() const
{
	glfwPollEvents();
}

void WindowManager::enableCursor() const
{
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void WindowManager::disableCursor() const
{
	glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

bool WindowManager::keyDown(const Key& key) const
{
	return glfwGetKey(mWindow, translateKey(key));
}

bool WindowManager::LMBDown() const
{
	return glfwGetMouseButton(mWindow, 0);
}

bool WindowManager::RMBDown() const
{
	return glfwGetMouseButton(mWindow, 1);
}

glm::vec2 WindowManager::cursorPosition() const
{
	double positionX, positionY;
	glfwGetCursorPos(mWindow, &positionX, &positionY);
	return glm::vec2(positionX, positionY);
}

void WindowManager::subscribeKeyPressEvent(const Key& key, const KeyCallback* callback)
{
	mKeyPressCallbacks[key].push_back((KeyCallback*)callback);
}

void WindowManager::unsubscribeKeyPressEvent(const Key& key, const KeyCallback* callback)
{
	std::vector<KeyCallback*>& callbacks = mKeyPressCallbacks[key];
	callbacks.erase(std::find(callbacks.begin(), callbacks.end(), (KeyCallback*)callback));
}

void WindowManager::subscribeKeyReleaseEvent(const Key& key, const KeyCallback* callback)
{
	mKeyReleaseCallbacks[key].push_back((KeyCallback*)callback);
}

void WindowManager::unsubscribeKeyReleaseEvent(const Key& key, const KeyCallback* callback)
{
	std::vector<KeyCallback*>& callbacks = mKeyReleaseCallbacks[key];
	callbacks.erase(std::find(callbacks.begin(), callbacks.end(), (KeyCallback*)callback));
}

void WindowManager::subscribeLMBPressedEvent(const MouseButtonCallback* callback)
{
	mLMBPressedCallbacks.push_back((MouseButtonCallback*)callback);
}

void WindowManager::unsubscribeLMBPressedEvent(const MouseButtonCallback* callback)
{
	mLMBPressedCallbacks.erase(std::find(mLMBPressedCallbacks.begin(), mLMBPressedCallbacks.end(), callback));
}

void WindowManager::subscribeRMBPressedEvent(const MouseButtonCallback* callback)
{
	mRMBPressedCallbacks.push_back((MouseButtonCallback*)callback);
}

void WindowManager::unsubscribeRMBPressedEvent(const MouseButtonCallback* callback)
{
	mRMBPressedCallbacks.erase(std::find(mRMBPressedCallbacks.begin(), mRMBPressedCallbacks.end(), callback));
}

void WindowManager::subscribeLMBReleasedEvent(const MouseButtonCallback* callback)
{
	mLMBReleasedCallbacks.push_back((MouseButtonCallback*)callback);
}

void WindowManager::unsubscribeLMBReleasedEvent(const MouseButtonCallback* callback)
{
	mLMBReleasedCallbacks.erase(std::find(mLMBReleasedCallbacks.begin(), mLMBReleasedCallbacks.end(), callback));
}

void WindowManager::subscribeRMBReleasedEvent(const MouseButtonCallback* callback)
{
	mRMBReleasedCallbacks.push_back((MouseButtonCallback*)callback);
}

void WindowManager::unsubscribeRMBReleasedEvent(const MouseButtonCallback* callback)
{
	mRMBReleasedCallbacks.erase(std::find(mRMBReleasedCallbacks.begin(), mRMBReleasedCallbacks.end(), callback));
}

void WindowManager::subscribeCursorMovedEvent(const CursorMovedCallback* callback)
{
	mCursorMovedCallbacks.push_back((CursorMovedCallback*)callback);
}

void WindowManager::unsubscribeCursorMovedEvent(const CursorMovedCallback* callback)
{
	mCursorMovedCallbacks.erase(std::find(mCursorMovedCallbacks.begin(), mCursorMovedCallbacks.end(), callback));
}
