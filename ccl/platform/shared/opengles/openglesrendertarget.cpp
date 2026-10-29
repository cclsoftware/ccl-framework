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

#include "ccl/gui/graphics/mutableregion.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/windows/window.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

using namespace CCL;

//************************************************************************************************
// OpenGLESRenderTarget
//************************************************************************************************

OpenGLESRenderTarget::OpenGLESRenderTarget ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

OpenGLESRenderTarget::~OpenGLESRenderTarget ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* OpenGLESRenderTarget::getSkiaCanvas ()
{
	sk_sp<SkSurface> surface = getSurface ();
	if(surface == nullptr)
	{
		CCL_PRINTF ("Creating a new surface: %dx%d\n", surfaceExtent.x, surfaceExtent.y)

		GrDirectContext* context = getGPUContext ();
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
			CCL_WARN ("%s\n", "Failed to create a Skia OpenGL ES surface");
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

void OpenGLESRenderTarget::renderWindow (Window& window, MutableRegion& invalidateRegion)
{
	if(!invalidateRegion.getRects ().isEmpty ())
	{
		makeCurrent ();
	
		render3DContent ();

		GrDirectContext* context = getGPUContext ();
		context->resetContext (kAll_GrBackendState);

		AutoPtr<NativeGraphicsDevice> nativeDevice = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createWindowDevice (&window));
		AutoPtr<GraphicsDevice> graphicsDevice = NEW WindowGraphicsDevice (window, nativeDevice);
		window.setGraphicsDevice (graphicsDevice);
		
		CCL_PROFILE_START (draw)
		
		// make sure to render 2D content behind transparent 3D surfaces
		for(OpenGLES3DSurface* surface : surfaces)
		{
			if(surface->getContent ()->getContentHint () != kGraphicsContentTranslucent)
				continue;
			RectRef surfaceRect = surface->getSize ();
			if(invalidateRegion.rectVisible (surfaceRect))
				invalidateRegion.addRect (surfaceRect);
		}
		
		for(int i = 0; i < invalidateRegion.getRects ().count (); i++)
		{
			RectRef invalidateRect = invalidateRegion.getRects ().at (i);
			
			graphicsDevice->saveState ();
			graphicsDevice->addClip (invalidateRect);
			if(isTranslucent ())
				graphicsDevice->clearRect (invalidateRect);
			
			window.setInDrawEvent (true);
			
			if(getOpacity () < 1.f)
			{
				SkPaint alpha;
				alpha.setAlphaf (getOpacity ());
				getSkiaCanvas ()->saveLayer (nullptr, &alpha);
			}
			
			window.draw (UpdateRgn (invalidateRect, &invalidateRegion));

			window.setInDrawEvent (false);
			graphicsDevice->restoreState ();
		}

		blendLayers (invalidateRegion);

		invalidateRegion.setEmpty ();
		
		window.setGraphicsDevice (nullptr);

		CCL_PROFILE_STOP (draw)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESRenderTarget::blendLayers (MutableRegion& invalidateRegion)
{
	// blend prerendered 3D surfaces to canvas
	for(OpenGLES3DSurface* surface : surfaces)
	{
		RectRef surfaceRect = surface->getSize ();
		if(!invalidateRegion.rectVisible (surfaceRect))
			continue;
		sk_sp<SkImage> image = surface->getSkiaImage ();
		if(image)
		{
			SkRect dstRect = SkRect::MakeLTRB (surfaceRect.left, surfaceRect.top, surfaceRect.right, surfaceRect.bottom);
			SkSamplingOptions samplingOptions (SkFilterMode::kLinear, SkMipmapMode::kLinear);
			getSkiaCanvas ()->drawImageRect (image, dstRect, samplingOptions);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESRenderTarget::flushSurface ()
{	
	GrDirectContext* context = getGPUContext ();
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

void OpenGLESRenderTarget::addOpenGLES3DSurface (Native3DSurface* surface)
{
	if(OpenGLES3DSurface* gles3DSurface = ccl_cast<OpenGLES3DSurface> (surface))
	{
		gles3DSurface->create (getGPUContext (), getScaleFactor ());
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
	GrDirectContext* context = getGPUContext ();
		
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
