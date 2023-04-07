#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include <map>
#include "Input.h"
#include "Time.h"

class Window
{
protected:
	int width = 800;
	int height = 600;
	GLFWwindow * window=0;
	Input input;

	static std::map<GLFWwindow *, Window*> allWindows;
	static void onKeyPressed(GLFWwindow *window, int key, int scancode, int action, int mod);
	static void onMouseButton(GLFWwindow *window, int button, int action, int mod);
	static void onScroll(GLFWwindow * window,double x,double y);

	static void onWindowResize(GLFWwindow *window, int w, int h);
	static void onWindowClose(GLFWwindow *window);
	static void onCursorPos(GLFWwindow *window, double x, double y);
	static void onResize(GLFWwindow *window, int x, int y);

	virtual void onUIUpdate();
	virtual void onRender();
	virtual void onUpdate();
	virtual void onResize(int w, int h);
	virtual void onInitialize();
	virtual void onClose();

	void makeWindowCurrent();
	virtual void onEndFrame();

public:
	Window(const char* caption);
	virtual ~Window();
	
	static bool shouldClose();
	static void run();
};
