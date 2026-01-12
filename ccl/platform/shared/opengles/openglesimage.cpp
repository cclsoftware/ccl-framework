//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2025 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/shared/opengles/openglesimage.cpp
// Description : OpenGLES Image
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/opengles/openglesimage.h"
#include "ccl/platform/shared/opengles/openglesclient.h"


#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace CCL;

//************************************************************************************************
// OpenGLESImage
//************************************************************************************************

OpenGLESImage::OpenGLESImage ()
: size {0, 0},
  sampleCount (1),
  textureId (0),
  framebufferId (0),
  format (GL_RGBA),
  mipLevels (1)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLESImage::~OpenGLESImage ()
{
	destroy ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESImage::create (const void* initialData)
{
	ASSERT (textureId == 0)
	ASSERT (mipLevels > 0)

	glGenTextures (1, &textureId);
	glBindTexture (GL_TEXTURE_2D, textureId);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to bind an image texture: %x\n", error)
	glTexImage2D (GL_TEXTURE_2D, 0, format, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, initialData);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to create a texture: %x\n", error)

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESImage::generateFramebuffer ()
{
	if(textureId == 0)
		return false;
	
	glBindTexture (GL_TEXTURE_2D, textureId);

	glGenFramebuffers (1, &framebufferId);
	glBindFramebuffer (GL_FRAMEBUFFER, framebufferId);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to bind a framebuffer: %x\n", error)
	glFramebufferTexture2D (GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureId, 0);
	if(GLenum error = glGetError ())
		CCL_WARN ("Failed to attach a framebuffer texture: %x\n", error)

	GLenum status = glCheckFramebufferStatus (GL_FRAMEBUFFER);
	if(status != GL_FRAMEBUFFER_COMPLETE)
	{
		CCL_WARN ("%s: %d\n", "Failed to create an OpenGL ES framebuffer", status);
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESImage::generateMipmaps ()
{
	if(textureId == 0)
		return false;
	
	glBindTexture (GL_TEXTURE_2D, textureId);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glGenerateMipmap (GL_TEXTURE_2D);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESImage::destroy ()
{
	if(textureId == 0)
		return;
	
	glFinish ();
	glDeleteTextures (1, &textureId);
	if(framebufferId != 0)
		glDeleteFramebuffers (1, &framebufferId);
	textureId = 0;
	framebufferId = 0;
}
