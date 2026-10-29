//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/shared/opengles/eglopenglesrendertarget.cpp
// Description : Skia Render Target using OpenGL ES and EGL
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/opengles/eglopenglesrendertarget.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace CCL;

//************************************************************************************************
// EGLOpenGLESRenderTarget
//************************************************************************************************

EGLOpenGLESRenderTarget::EGLOpenGLESRenderTarget ()
: eglSurface (EGL_NO_SURFACE)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

EGLOpenGLESRenderTarget::~EGLOpenGLESRenderTarget ()
{
	OpenGLESClient& client = OpenGLESClient::instance ();
	if(eglSurface != EGL_NO_SURFACE)
	{
		glFinish ();
		eglDestroySurface (client.getDisplay (), eglSurface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EGLOpenGLESRenderTarget::initializeSurface (EGLNativeWindowType window)
{
	ASSERT (eglSurface == EGL_NO_SURFACE)
	
	OpenGLESClient& client = OpenGLESClient::instance ();

	CCL_PRINTF ("Creating EGL window surface with display %x and config %x for window %x\n", client.getDisplay (), client.getConfig (), window);
	eglSurface = eglCreateWindowSurface (client.getDisplay (), client.getConfig (), window, nullptr);
	if(eglSurface == EGL_NO_SURFACE)
	{
		CCL_WARN ("%s\n", "Failed to create an EGL window surface");
		return;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* EGLOpenGLESRenderTarget::getSkiaCanvas ()
{
	ASSERT (eglSurface != EGL_NO_SURFACE)
	return OpenGLESRenderTarget::getSkiaCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EGLOpenGLESRenderTarget::presentFrame ()
{
	OpenGLESClient& client = OpenGLESClient::instance ();

	eglSwapBuffers (client.getDisplay (), eglSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EGLOpenGLESRenderTarget::makeCurrent ()
{
	OpenGLESClient& client = OpenGLESClient::instance ();
	EGLContext eglContext = client.getContext ();
	if(eglSurface == EGL_NO_SURFACE || eglContext == EGL_NO_CONTEXT)
		return;

	eglMakeCurrent (client.getDisplay (), eglSurface, eglSurface, eglContext);
	
	EGLint error = eglGetError ();
	if(error != EGL_SUCCESS)
		CCL_WARN ("Failed to switch egl context: %x\n", error)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

GrDirectContext* EGLOpenGLESRenderTarget::getGPUContext ()
{
	return OpenGLESClient::instance ().getGPUContext ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void EGLOpenGLESRenderTarget::addOpenGLES3DSurface (Native3DSurface* surface)
{
	if(graphicsContext3D == nullptr)
		OpenGLES3DSupport::instance ().setGLExtensions (OpenGLESClient::instance ().getGLExtensions ());

	OpenGLESRenderTarget::addOpenGLES3DSurface (surface);
}