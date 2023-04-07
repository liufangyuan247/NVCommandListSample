#pragma once
#include <GLFW/glfw3.h>
#include <vector>

class Input
{
	bool oldKeyState[GLFW_KEY_LAST+1];
	bool keyState[GLFW_KEY_LAST+1];
	bool oldButtonState[16];
	bool buttonState[16];

	float lastX;
	float lastY;
	float _x;
	float _y;
	float scrollX_;
	float scrollY_;
	bool uiBlocked;

public:
	Input();
	void onKey(int key, int scancode, int action, int mod);
	void onMouseButton(int button, int action, int mod);
	void onMouseMove(float x, float y);
	void onUIBlocked(bool blocked);
	void onMouseScroll(double xScroll, double yScroll);

	bool getButton(int button);
	bool getButtonDown(int button);
	bool getButtonUp(int button);
	bool getKey(int key);
	bool getKeyDown(int key);
	bool getKeyUp(int key);

	float scrollX();
	float scrollY();

	bool Shift();
	bool Ctrl();
	bool Alt();
	bool blockedByUI();

	void endFrame();
	float x() const;
	float y() const;
	float deltaX() const;
	float deltaY() const;
};
