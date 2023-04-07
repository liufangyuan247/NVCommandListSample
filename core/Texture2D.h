#pragma once
#include <GL/glew.h>
#include "GLObject.h"

class Texture2D : public GLObject
{
	friend class FrameBuffer;
protected:
	GLenum internalFormat;
	int w, h;
	int levels;
	GLenum wrapS,wrapT;
	GLenum minFilter,magFilter;

public:
	operator bool();

	Texture2D();
	~Texture2D();

	Texture2D(Texture2D &&other) = default;
	Texture2D &operator=(Texture2D &&other) = default;

	void Create(int levels, int w, int h, GLenum internalFormat, GLenum format, GLenum type, void *data);
	void SetTextureFilter(GLenum magfilter, GLenum minFilter);
	void SetTextureWrapMod(GLenum wrapS, GLenum wrapT);
	void SetBorderColor(float *color);
	void Resize(int w, int h);
	void FillData(int level, int x, int y, int w, int h, GLenum format, GLenum type, void *data);
	void Bind(int location);

	void ReadData(void *buffer, int x, int y, int w, int h, GLenum format, GLenum type);

	static void ReadScreenData(void *buffer, int x, int y, int w, int h, GLenum format, GLenum type);

	Texture2D Clone();

	void Unbind();
	void Destroy();

	int Width();
	int Height();
	int Levels();
	GLenum InternalFormat();
};
