#include "Input.h"
#include <cstdio>

Input::Input()
{
	for (auto &s : keyState)
	{
		s = false;
	}
	for (auto &s : oldKeyState)
	{
		s = false;
	}
	for (auto &s : buttonState)
	{
		s = false;
	}
	for (auto &s : oldButtonState)
	{
		s = false;
	}
	lastX = 0;
	lastY = 0;
	_x = 0;
	_y = 0;
	uiBlocked = false;
}

void Input::onKey(int key, int scancode, int action, int mod)
{
	if (key == GLFW_KEY_UNKNOWN)
	{
		return;
	}
	keyState[key] = action != GLFW_RELEASE;
}

void Input::onMouseMove(float x, float y)
{
	_x = x;
	_y = y;
}

void Input::onUIBlocked(bool blocked)
{
	uiBlocked = blocked;
}

bool Input::getButton(int button)
{
	return buttonState[button];
}

bool Input::getButtonDown(int button)
{
	return (!oldButtonState[button]) && buttonState[button];
}

bool Input::getButtonUp(int button)
{
	return (!buttonState[button]) && oldButtonState[button];
}

bool Input::getKey(int key)
{
	return keyState[key];
}

bool Input::getKeyDown(int key)
{
	return (!oldKeyState[key]) && keyState[key];
}

bool Input::getKeyUp(int key)
{
	return (!keyState[key]) && oldKeyState[key];
}

bool Input::Shift()
{
	return getKey(GLFW_KEY_LEFT_SHIFT) || getKey(GLFW_KEY_RIGHT_SHIFT);
}

bool Input::Ctrl()
{
	return getKey(GLFW_KEY_LEFT_CONTROL) || getKey(GLFW_KEY_RIGHT_CONTROL);
}

bool Input::Alt()
{
	return getKey(GLFW_KEY_LEFT_ALT) || getKey(GLFW_KEY_RIGHT_ALT);
}

bool Input::blockedByUI()
{
	return uiBlocked;
}

void Input::onMouseScroll(double xScroll, double yScroll)
{
	scrollX_ = xScroll;
	scrollY_ = yScroll;
}

float Input::scrollX()
{
	return scrollX_;
}

float Input::scrollY()
{
	return scrollY_;
}

void Input::onMouseButton(int button, int action, int mod)
{
	buttonState[button] = action != GLFW_RELEASE;
}

void Input::endFrame()
{
	for (int i = 0; i < std::size(keyState); ++i)
	{
		oldKeyState[i] = keyState[i];
	}

	for (int i = 0; i < std::size(buttonState); ++i)
	{
		oldButtonState[i] = buttonState[i];
	}

	lastX = _x;
	lastY = _y;
	scrollX_ = 0;
	scrollY_ = 0;
}

float Input::x() const
{
	return _x;
}

float Input::y() const
{
	return _y;
}

float Input::deltaX() const
{
	return _x - lastX;
}

float Input::deltaY() const
{
	return _y - lastY;
}
