#pragma once
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <vector>
#include <functional>

#define WINDOW_TITLE "Snake"
#define DEFAULT_DISABLE_CURSOR true

// Defines the keys that may be used to recieve input
enum Key : unsigned int { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Numeric0, Numeric1, Numeric2, Numeric3, Numeric4, Numeric5, Numeric6, Numeric7, Numeric8, Numeric9,
	Space, LeftBracket, RightBracket, Semicolon, Apostrophe, Backslash, Slash, Comma, Dot, COUNT, UNKNOWN };

typedef std::function<void()> KeyCallback, MouseButtonCallback;
typedef std::function<void(const glm::vec2&, const glm::vec2&)> CursorMovedCallback;

class WindowManager
{
private:
	friend class RenderSystem;
	friend void keyCallback(GLFWwindow*, int, int, int, int);
	friend void mouseButtonCallback(GLFWwindow*, int, int, int);
	friend void cursorMovedCallback(GLFWwindow*, double, double);

	GLFWwindow* mWindow = nullptr;
	const char* mWindowTitle;
	unsigned int mWindowWidth;
	unsigned int mWindowHeight;

	std::vector<KeyCallback*> mKeyPressCallbacks[COUNT];
	std::vector<KeyCallback*> mKeyReleaseCallbacks[COUNT];

	std::vector<MouseButtonCallback*> mLMBPressedCallbacks;
	std::vector<MouseButtonCallback*> mRMBPressedCallbacks;
	std::vector<MouseButtonCallback*> mLMBReleasedCallbacks;
	std::vector<MouseButtonCallback*> mRMBReleasedCallbacks;

	std::vector<CursorMovedCallback*> mCursorMovedCallbacks;

	WindowManager(const char* windowTitle, const unsigned int& windowWidth, const unsigned int& windowHeight);

public:
	static WindowManager& instance();

	WindowManager(const WindowManager& copy) = delete;
	~WindowManager();

	bool windowClosed() const;
	void pollEvents() const;

	void enableCursor() const;
	void disableCursor() const;

	bool keyDown(const Key& key) const;
	bool LMBDown() const;
	bool RMBDown() const;

	glm::vec2 cursorPosition() const;

	void subscribeKeyPressEvent(const Key& key, const KeyCallback* callback);
	void unsubscribeKeyPressEvent(const Key& key, const KeyCallback* callback);
	
	void subscribeKeyReleaseEvent(const Key& key, const KeyCallback* callback);
	void unsubscribeKeyReleaseEvent(const Key& key, const KeyCallback* callback);

	void subscribeLMBPressedEvent(const MouseButtonCallback* callback);
	void unsubscribeLMBPressedEvent(const MouseButtonCallback* callback);
	void subscribeRMBPressedEvent(const MouseButtonCallback* callback);
	void unsubscribeRMBPressedEvent(const MouseButtonCallback* callback);

	void subscribeLMBReleasedEvent(const MouseButtonCallback* callback);
	void unsubscribeLMBReleasedEvent(const MouseButtonCallback* callback);
	void subscribeRMBReleasedEvent(const MouseButtonCallback* callback);
	void unsubscribeRMBReleasedEvent(const MouseButtonCallback* callback);
	
	void subscribeCursorMovedEvent(const CursorMovedCallback* callback);
	void unsubscribeCursorMovedEvent(const CursorMovedCallback* callback);

	inline const unsigned int& getWindowWidth() const
	{
		return mWindowWidth;
	}

	inline const unsigned int& getWindowHeight() const
	{
		return mWindowHeight;
	}
};