#include "FrameBuffer.h"
#include <cstdio>

FrameBuffer::operator bool()
{
	return isComplete();
}

FrameBuffer::FrameBuffer()
{
}

FrameBuffer::~FrameBuffer()
{
	Destroy();
}

void FrameBuffer::Create()
{
	glCreateFramebuffers(1, &id);
}

void FrameBuffer::AttachColorTexture(Texture2D *texture, int attachmentId)
{
	w = texture->Width();
	h = texture->Height();
	glNamedFramebufferTexture(id, GL_COLOR_ATTACHMENT0 + attachmentId, texture->id, 0);
}

void FrameBuffer::AttachDepthTexture(Texture2D *texture)
{
	w = texture->Width();
	h = texture->Height();
	glNamedFramebufferTexture(id, GL_DEPTH_ATTACHMENT, texture->id, 0);
}

void FrameBuffer::AttachDepthStencilTexture(Texture2D *texture)
{
	w = texture->Width();
	h = texture->Height();
	glNamedFramebufferTexture(id, GL_DEPTH_STENCIL_ATTACHMENT, texture->id, 0);
}

void FrameBuffer::DrawBuffers(GLenum *buffers, int count)
{
	glNamedFramebufferDrawBuffers(id, count, buffers);
}

bool FrameBuffer::isComplete()
{
	GLenum status = glCheckNamedFramebufferStatus(id, GL_FRAMEBUFFER);
	return status == GL_FRAMEBUFFER_COMPLETE;
}

void FrameBuffer::Bind(GLenum framebuffer_bind_pos)
{
	glBindFramebuffer(framebuffer_bind_pos, id);
	glViewport(0, 0, w, h);
}

void FrameBuffer::Unbind()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FrameBuffer::Destroy()
{
	if (id)
	{
		glDeleteFramebuffers(1, &id);
		id = 0;
	}
}

int FrameBuffer::Width()
{
	return w;
}

int FrameBuffer::Height()
{
	return h;
}
