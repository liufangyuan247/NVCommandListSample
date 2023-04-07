#pragma once
#include <GL/glew.h>
#include "imgui/imgui.h"
#include "core/stb_image.h"

static int stack_depth = 0;

struct ImguiIdHelper
{
	template <typename T>
	ImguiIdHelper(T &&id)
	{
		ImGui::PushID(id);
		// stack_depth+=1;
		// printf("push id ,depth:%d\n",stack_depth);
	}
	~ImguiIdHelper()
	{
		ImGui::PopID();
		// stack_depth-=1;
		// printf("pop id ,depth:%d\n",stack_depth);
	}
};

#define CONCAT_DIRECT(X, Y) X##Y
#define CONCAT(X, Y) CONCAT_DIRECT(X, Y)
#define UNIQUE_NAME_LINE CONCAT(unique_name_, __LINE__) // helper macro
#define NewImGuiIDHelper(id) ImguiIdHelper UNIQUE_NAME_LINE(id)

namespace ImGui
{
	ImVec2 GetWindowPosFromScreen(ImVec2 screenPos);
	ImVec2 GetScreenPosFromWindow(ImVec2 windowPos);
}

template <GLenum state>
class ToggleState
{
	GLboolean enabled;

public:
	explicit ToggleState()
	{
		enabled = glIsEnabled(state);
	}
	~ToggleState()
	{
		enabled ? glEnable(state) : glDisable(state);
	}
};

struct ViewPortGuard
{
	int oldViewport[4];
	ViewPortGuard()
	{
		glGetIntegerv(GL_VIEWPORT, oldViewport);
	}
	~ViewPortGuard()
	{
		glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
	}
};

struct BlendGuard
{
	ToggleState<GL_BLEND> s;
	int rgb_sfactor;
	int rgb_dfactor;
	int alpha_sfactor;
	int alpha_dfactor;
	int rgb_equation;
	int alpha_equation;
	BlendGuard()
	{
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &alpha_sfactor);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &alpha_dfactor);
		glGetIntegerv(GL_BLEND_SRC_RGB, &rgb_sfactor);
		glGetIntegerv(GL_BLEND_DST_RGB, &rgb_dfactor);
		glGetIntegerv(GL_BLEND_EQUATION_RGB, &rgb_equation);
		glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &alpha_equation);
	}
	~BlendGuard()
	{
		glBlendFuncSeparate(rgb_sfactor, rgb_dfactor, alpha_sfactor, alpha_dfactor);
		glBlendEquationSeparate(rgb_equation, alpha_equation);
	}
};

struct LogicOpGuard
{
	ToggleState<GL_COLOR_LOGIC_OP> s;
	int mode;
	LogicOpGuard()
	{
		glGetIntegerv(GL_LOGIC_OP_MODE, &mode);
	}
	~LogicOpGuard()
	{
		glLogicOp(mode);
	}
};

struct BufferMaskGuard
{
	int colorMask[4];
	int depthMask;
	BufferMaskGuard()
	{
		glGetIntegerv(GL_COLOR_WRITEMASK, colorMask);
		glGetIntegerv(GL_DEPTH_WRITEMASK, &depthMask);
	}
	~BufferMaskGuard()
	{
		glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);
		glDepthMask(depthMask);
	}
};

struct ScissorGuard
{
	int x, y, w, h;
	ToggleState<GL_SCISSOR_TEST> s;

	ScissorGuard()
	{
		int data[4];
		glGetIntegerv(GL_SCISSOR_BOX, data);
		x = data[0];
		y = data[1];
		w = data[2];
		h = data[3];
	}
	~ScissorGuard()
	{
		glScissor(x, y, w, h);
	}
};

struct StencilGuard
{
	int func, ref, mask;
	int fail, zfail, zpass;
	ToggleState<GL_STENCIL_TEST> s;

	StencilGuard()
	{
		glGetIntegerv(GL_STENCIL_FAIL, &fail);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &zfail);
		glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &zpass);
		glGetIntegerv(GL_STENCIL_FUNC, &fail);
		glGetIntegerv(GL_STENCIL_REF, &ref);
		glGetIntegerv(GL_STENCIL_VALUE_MASK, &mask);
	}
	~StencilGuard()
	{
		glStencilFunc(func, ref, mask);
		glStencilOp(fail, zfail, zpass);
	}
};

struct FramebufferGuard
{
	int read_fbo;
	int draw_fbo;
	FramebufferGuard()
	{
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read_fbo);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw_fbo);
	}

	~FramebufferGuard()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
	}
};

void ClearGLError();

void PrintGLError();

unsigned int GetDrivesBitMask();

static const float cm2inch = 0.3937007874f;
static const float inch2cm = 2.54f;

static const int CM = 0;
static const int INCH = 1;
