#pragma once
class Time
{
	static float _time;
	static float _lastTime;
public:
	static float time();
	static float deltaTime();
	static void startFrame();
	static void endFrame();
};
