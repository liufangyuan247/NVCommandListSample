#include "core/Window.h"
#include "app/PDWindow.h"
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <cstdlib>
#include <fstream>

#include "app/json.hpp"

#ifdef _WIN32
#ifndef _DEBUG
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
int main(int, const char **)
#endif
#else
int main(int argc, const char **argv)
#endif
{
	glfwInit();
	auto w1 = new PDWindow();
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	(void)io;
	io.Fonts->AddFontFromFileTTF("assets//font//Deng.ttf", 15, NULL, io.Fonts->GetGlyphRangesChineseFull());
	ImGui::StyleColorsLight();
	Window::run();

	glfwTerminate();
	return 0;
}