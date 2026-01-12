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
// Filename    : ccl/platform/shared/opengles/openglesrendertarget.cpp
// Description : Skia Render Target using OpenGL ES
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/shared/opengles/openglesrendertarget.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace CCL;

//************************************************************************************************
// OpenGLESRenderTarget
//************************************************************************************************

OpenGLESRenderTarget::OpenGLESRenderTarget ()
: eglSurface (EGL_NO_SURFACE)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLESRenderTarget::~OpenGLESRenderTarget ()
{
	OpenGLESClient& client = OpenGLESClient::instance ();
	if(eglSurface != EGL_NO_SURFACE)
	{
		glFinish ();
		eglDestroySurface (client.getDisplay (), eglSurface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESRenderTarget::initializeSurface (EGLNativeWindowType window)
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

SkCanvas* OpenGLESRenderTarget::getSkiaCanvas ()
{
	if(eglSurface == nullptr)
		initialize ();

	sk_sp<SkSurface> surface = getSurface ();
	if(surface == nullptr)
	{
		CCL_PRINTF ("Creating a new surface: %dx%d\n", surfaceExtent.x, surfaceExtent.y)

		GrDirectContext* context = OpenGLESClient::instance ().getGPUContext ();
		if(context == nullptr)
			return nullptr;

		if(textureImage != nullptr)
		{
			glFinish ();
			textureImage = nullptr;
			targetSurface = nullptr;
			texture.destroy ();
		}

		texture.setSize (surfaceExtent);
		texture.create ();
		texture.generateFramebuffer ();
		
		GrGLFramebufferInfo frameBufferInfo;
		frameBufferInfo.fFBOID = texture.getFramebufferID ();
		frameBufferInfo.fFormat = GL_RGBA8_OES;

		GrBackendRenderTarget targetInfo = GrBackendRenderTargets::MakeGL (surfaceExtent.x, surfaceExtent.y, 0, 8, frameBufferInfo);

		surface = SkSurfaces::WrapBackendRenderTarget (context, targetInfo, kBottomLeft_GrSurfaceOrigin, SkColorType::kRGBA_8888_SkColorType, nullptr, nullptr);
		if(surface == nullptr)
		{
			CCL_WARN ("%s\n", "Failed to create a Skia OpenGL ES framebuffer surface");
			return nullptr;
		}

		SkCanvas* canvas = surface->getCanvas ();
		canvas->scale (getScaleFactor (), getScaleFactor ());
		canvas->clear (SkColorSetARGB (0, 0, 0, 0));

		frameBufferInfo.fFBOID = 0;
		targetInfo = GrBackendRenderTargets::MakeGL (surfaceExtent.x, surfaceExtent.y, 0, 8, frameBufferInfo);

		targetSurface = SkSurfaces::WrapBackendRenderTarget (context, targetInfo, kBottomLeft_GrSurfaceOrigin, SkColorType::kRGBA_8888_SkColorType, nullptr, nullptr);
		if(targetSurface == nullptr)
		{
			CCL_WARN ("%s\n", "Failed to create a Skia OpenGL ES window surface");
			return nullptr;
		}

		GrGLTextureInfo textureInfo = {0};
		textureInfo.fTarget = GL_TEXTURE_2D;
		textureInfo.fID = texture.getTextureID ();
		textureInfo.fFormat = GL_RGBA8_OES;

		GrBackendTexture texture = GrBackendTextures::MakeGL (surfaceExtent.x, surfaceExtent.y, skgpu::Mipmapped::kNo, textureInfo);
		textureImage = SkImages::BorrowTextureFrom (context, texture, kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, kOpaque_SkAlphaType, nullptr);
		if(textureImage == nullptr)
		{
			CCL_WARN ("%s\n", "Failed to create a Skia image from a backend texture");
			return nullptr;
		}

		setSurface (surface);
	}
	return getSurface () ? getSurface ()->getCanvas () : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESRenderTarget::flushSurface ()
{
	GrDirectContext* context = OpenGLESClient::instance ().getGPUContext ();
	if(context == nullptr)
		return false;

	// flush render commands and make sure the intermediate buffer is valid
	sk_sp<SkSurface> surface = getSurface ();
	ASSERT (surface != nullptr)
	surface->recordingContext ()->asDirectContext ()->flushAndSubmit (surface.get ());

	// flush the offscreen texture
	ASSERT (textureImage != nullptr)
	context->flushAndSubmit (textureImage);

	// draw the offscreen texture to the window surface and flush render commands
	ASSERT (targetSurface != nullptr)
	SkRect dstRect = SkRect::MakeWH (surfaceExtent.x, surfaceExtent.y);
	
	SkPaint paint;
	paint.setBlendMode (SkBlendMode::kSrc);
	targetSurface->getCanvas ()->drawImageRect (textureImage, dstRect, SkSamplingOptions (), &paint);
	targetSurface->recordingContext ()->asDirectContext ()->flushAndSubmit (targetSurface.get ());

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESRenderTarget::presentFrame ()
{
	OpenGLESClient& client = OpenGLESClient::instance ();

	eglSwapBuffers (client.getDisplay (), eglSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESRenderTarget::makeCurrent ()
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

void OpenGLESRenderTarget::addOpenGLES3DSurface (Native3DSurface* surface)
{
	if(OpenGLES3DSurface* gles3DSurface = ccl_cast<OpenGLES3DSurface> (surface))
	{
		gles3DSurface->create (OpenGLESClient::instance ().getGPUContext (), getScaleFactor ());
		surfaces.add (gles3DSurface);
	}

	if(graphicsContext3D == nullptr)
		graphicsContext3D = NEW OpenGLES3DGraphicsContext;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESRenderTarget::removeOpenGLES3DSurface (Native3DSurface* surface)
{
	for(int i = 0; i < surfaces.count (); i++)
	{
		if(surfaces[i] == ccl_cast<OpenGLES3DSurface> (surface))
		{
			surfaces[i]->destroy ();
			surfaces.removeAt (i);
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESRenderTarget::render3DContent ()
{
	GrDirectContext* context = OpenGLESClient::instance ().getGPUContext ();
		
	if(graphicsContext3D == nullptr)
		return;

	for(OpenGLES3DSurface* surface : surfaces)
	{
		if(!surface->isValid () && !surface->create (context, getScaleFactor ()))
			continue;
		surface->render (*graphicsContext3D);
		context->flushAndSubmit (surface->getSkiaImage ());
	}
}
