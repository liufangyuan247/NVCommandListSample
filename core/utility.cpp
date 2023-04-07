#include "utility.h"
#include "cstdio"
#include "core/stb_image.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;

ImVec2 ImGui::GetWindowPosFromScreen(ImVec2 screenPos)
{
	return ImVec2(screenPos.x- ImGui::GetWindowPos().x + ImGui::GetScrollX(), screenPos.y - ImGui::GetWindowPos().y + ImGui::GetScrollY());
}
ImVec2 ImGui::GetScreenPosFromWindow(ImVec2 windowPos)
{
	return ImVec2(windowPos.x + ImGui::GetWindowPos().x - ImGui::GetScrollX(), windowPos.y + ImGui::GetWindowPos().y - ImGui::GetScrollY());
}
void ClearGLError()
{
	while (glGetError());
}

void PrintGLError()
{
	int idx = 0;
	while (true)
	{
		auto error = glGetError();
		if (!error)
		{
			break;
		}
		printf("GLError%d:%d\n", idx, error);
	}
}

unsigned int GetDrivesBitMask()
{
#ifdef _WIN32
	DWORD mask = GetLogicalDrives();
	unsigned int ret = 0;
	for (int i = 0; i < 26; ++i)
	{
		if (!(mask & (1 << i)))
			continue;
		char rootName[4] = { static_cast<char>('A' + i), ':', '\\', '\0' };
		UINT type = GetDriveTypeA(rootName);
		if (type == DRIVE_REMOVABLE || type == DRIVE_FIXED)
			ret |= (1 << i);
	}
	return ret;
#else
	return 0;
#endif
}
