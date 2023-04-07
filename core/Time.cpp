#include "Time.h"
#include <GLFW/glfw3.h>

float Time::_time=0;
float Time::_lastTime=0;

float Time::time()
{
	return _time;
}

float Time::deltaTime()
{
	return _time-_lastTime;
}

void Time::startFrame()
{
	_time = glfwGetTime();
}

void Time::endFrame()
{
	_lastTime = _time;
}
