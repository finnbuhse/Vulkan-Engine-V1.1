#pragma once
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <vector>
#include <functional>

enum Key : unsigned int { A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z, Numeric0, Numeric1, Numeric2, Numeric3, Numeric4, Numeric5, Numeric6, Numeric7, Numeric8, Numeric9,
	Space, LeftBracket, RightBracket, Semicolon, Apostrophe, Backslash, Slash, Comma, Dot, COUNT, UNKNOWN };

class WindowManager
{
private:
	friend class RenderSystem;
	friend void keyCallback(GLFWwindow*, int, int, int, int);
	friend void cursorMovedCallback(GLFWwindow*, double, double);

	GLFWwindow* mWindow = nullptr;
	const char* mWindowTitle;
	unsigned int mWindowWidth;
	unsigned int mWindowHeight;

	std::vector<std::function<void()>> mKeyPressCallbacks[COUNT];
	std::vector<std::function<void()>> mKeyReleaseCallbacks[COUNT];
	std::vector<std::function<void(const glm::vec2&, const glm::vec2&)>> mCursorMovedCallbacks;

	WindowManager(const char* windowTitle, const unsigned int& windowWidth, const unsigned int& windowHeight);

public:
	static WindowManager& instance();

	WindowManager(const WindowManager& copy) = delete;
	~WindowManager();

	bool windowClosed() const;
	void pollEvents() const;

	bool keyDown(const Key& key) const;
	glm::vec2 cursorPosition() const;

	std::vector<std::function<void()>>::iterator subscribeKeyPressEvent(const Key& key, const std::function<void()>& callback);
	void unsubscribeKeyPressEvent(const Key& key, const std::vector<std::function<void()>>::iterator& iterator);
	std::vector<std::function<void()>>::iterator subscribeKeyReleaseEvent(const Key& key, const std::function<void()>& callback);
	void unsubscribeKeyReleaseEvent(const Key& key, const std::vector<std::function<void()>>::iterator& iterator);
	std::vector<std::function<void(const glm::vec2&, const glm::vec2&)>>::iterator subscribeCursorMovedEvent(const std::function<void(const glm::vec2&, const glm::vec2&)>& callback);
	void unsubscribeCursorMovedEvent(const std::vector<std::function<void(const glm::vec2&, const glm::vec2&)>>::iterator& iterator);
};