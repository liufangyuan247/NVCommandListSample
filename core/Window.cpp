#include "Window.h"

std::map<GLFWwindow *, Window*> Window::allWindows;


void Window::onKeyPressed(GLFWwindow * window, int key, int scancode, int action, int mod)
{
	if (allWindows.count(window))
	{
		allWindows[window]->input.onKey(key, scancode, action, mod);
	}
}

void Window::onMouseButton(GLFWwindow * window, int button, int action, int mod)
{
	if (allWindows.count(window))
	{
		allWindows[window]->input.onMouseButton(button, action, mod);
	}
}

void Window::onScroll(GLFWwindow * window,double x,double y)
{
	if (allWindows.count(window))
	{
		allWindows[window]->input.onMouseScroll(x, y);
	}
}

void Window::onWindowResize(GLFWwindow * window, int w, int h)
{
	if (allWindows.count(window))
	{
		allWindows[window]->onResize(w, h);
	}
}

void Window::onWindowClose(GLFWwindow * window)
{
	auto iter = allWindows.find(window);
	if (iter != allWindows.end())
	{
		iter->second->onClose();
		delete iter->second;
		allWindows.erase(iter);
	}
	allWindows.clear();
}

void Window::onCursorPos(GLFWwindow * window, double x, double y)
{
	if (allWindows.count(window))
	{
		allWindows[window]->input.onMouseMove(x, y);
	}
}

void Window::onResize(GLFWwindow * window, int x, int y)
{
	if (allWindows.count(window))
	{
		allWindows[window]->onResize(x, y);
	}
}

Window::Window(const char* caption)
{
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_SAMPLES, 4);

	window = glfwCreateWindow(width, height, caption, 0, 0);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	glewExperimental=true;
	glewInit();

	allWindows[window] = this;
	glfwSetKeyCallback(window, onKeyPressed);
	glfwSetMouseButtonCallback(window, onMouseButton);
	glfwSetScrollCallback(window, onScroll);
	glfwSetWindowSizeCallback(window, onResize);
	glfwSetCursorPosCallback(window, onCursorPos);
	glfwSetWindowCloseCallback(window, onWindowClose);
	glfwShowWindow(window);
}

Window::~Window()
{
	auto iter = allWindows.find(window);
	if (iter != allWindows.end())
	{
		allWindows.erase(iter);
	}
}

void Window::onUIUpdate()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	input.onUIBlocked(ImGui::GetIO().WantCaptureKeyboard || ImGui::GetIO().WantCaptureMouse);
}

void Window::onRender()
{

}

void Window::onUpdate()
{
}

void Window::onResize(int w, int h)
{
	width = w;
	height = h;
}

void Window::onInitialize()
{
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
}

void Window::onClose()
{
	glfwDestroyWindow(window);
	window = nullptr;
}

void Window::makeWindowCurrent()
{
	glfwMakeContextCurrent(window);
}

void Window::onEndFrame()
{
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	input.endFrame();
}

bool Window::shouldClose()
{
	return allWindows.empty();
}

void Window::run()
{
	for (auto &w:allWindows)
	{
		glfwMakeContextCurrent(w.first);
		w.second->onInitialize();
	}
	while (!shouldClose())
	{
		glfwPollEvents();
		Time::startFrame();
		for (auto &w : allWindows)
		{
			Window * cw = w.second;
			cw->makeWindowCurrent();
			// Start the Dear ImGui frame
			cw->onUIUpdate();
			ImGui::Render();
			cw->onUpdate();
			cw->onRender();
			cw->onEndFrame();
			glfwSwapBuffers(w.first);
		}
		Time::endFrame();
	}
}
