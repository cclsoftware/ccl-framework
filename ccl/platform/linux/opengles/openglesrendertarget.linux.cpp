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
// Filename    : ccl/platform/linux/opengles/openglesrendertarget.linux.cpp
// Description : Skia Render Target using OpenGL ES and Wayland
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/linux/opengles/openglesrendertarget.linux.h"
#include "ccl/platform/linux/gui/window.linux.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// LinuxOpenGLESRenderTarget
//************************************************************************************************

LinuxOpenGLESRenderTarget::LinuxOpenGLESRenderTarget ()
: eglWindow (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxOpenGLESRenderTarget::~LinuxOpenGLESRenderTarget ()
{
	if(!WaylandClient::instance ().isInitialized ())
	{
		// we lost previously allocated compositor objects, trying to destroy those could freeze or crash the application
		eglWindow = nullptr;
		eglSurface = EGL_NO_SURFACE;
	}

	if(eglWindow)
		wl_egl_window_destroy (eglWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxOpenGLESRenderTarget::initialize (wl_surface* surface)
{
	setWaylandSurface (surface);
	
	// create EGL window from Wayland surface
	eglWindow = wl_egl_window_create (surface, ccl_max (surfaceExtent.x, 1), ccl_max (surfaceExtent.y, 1));
	if(eglWindow == nullptr)
	{
		CCL_WARN ("%s\n", "Failed to create an EGL window from a Wayland surface")
		return false;
	}
	
	initializeSurface (eglWindow);
	
	return getSurface () != nullptr;
}

//************************************************************************************************
// OpenGLESWindowRenderTarget
//************************************************************************************************

OpenGLESWindowRenderTarget::OpenGLESWindowRenderTarget (Window& window)
: SkiaWindowRenderTarget (window),
  linuxWindow (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESWindowRenderTarget::initialize ()
{
	linuxWindow = LinuxWindow::cast (&window);
	return LinuxOpenGLESRenderTarget::initialize (linuxWindow->getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::clear ()
{
	updateRegion.setEmpty ();
	invalidateRegion.setEmpty ();
	invalidateRegion.addRect (Rect (0, 0, size.x, size.y));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::onSize ()
{
	if(scaleFactor != window.getContentScaleFactor ())
		onContentScaleFactorChanged (window.getContentScaleFactor ());

	size = PixelPoint (Point (window.getWidth (), window.getHeight ()), window.getContentScaleFactor ());
	
	if(eglSurface == nullptr)
	{
		applyContentScaleFactor ();
		applySize ();
		initialize ();
	}
	
	if(!linuxWindow->isConfigured ())
	{
		if(listener)
			delete listener;
		listener = nullptr;
		return;
	}

	if(listener == nullptr && linuxWindow->wantsFrameCallback ())
	{
		listener = NEW Listener (this);
		onPresent ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::applySize ()
{
	surface = nullptr;
	surfaceExtent = size;
	if(eglWindow)
		wl_egl_window_resize (eglWindow, surfaceExtent.x, surfaceExtent.y, 0, 0);
	clear ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* OpenGLESWindowRenderTarget::getCanvas ()
{
	SkCanvas* canvas = OpenGLESRenderTarget::getSkiaCanvas ();
	if(canvas == nullptr)
	{
		//TODO either the graphics device or the wayland surface is gone
	}
	return canvas;

}

//////////////////////////////////////////////////////////////////////////////////////////////////

float OpenGLESWindowRenderTarget::getOpacity () const
{
	return window.getOpacity ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESWindowRenderTarget::isTranslucent () const
{
	return window.getStyle ().isTranslucent () || getOpacity () < 1.f;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::onRender ()
{
	if(!invalidateRegion.getRects ().isEmpty ())
	{
		makeCurrent ();
	
		render3DContent ();

		GrDirectContext* context = OpenGLESClient::instance ().getGPUContext ();
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
				invalidateRegion.addRect (surfaceRect, false);
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
				getCanvas ()->saveLayer (nullptr, &alpha);
			}
			
			window.draw (UpdateRgn (invalidateRect, &invalidateRegion));

			window.setInDrawEvent (false);
			graphicsDevice->restoreState ();
		}

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
				getCanvas ()->drawImageRect (image, dstRect, samplingOptions);
			}
		}
		
		invalidateRegion.setEmpty ();
		
		CCL_PROFILE_STOP (draw)
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::onPresent ()
{
	if(size.x != surfaceExtent.x || size.y != surfaceExtent.y)
		applySize ();
			
	onRender ();
	if(surface)
	{
		if(flushSurface ())
			presentFrame ();
	}

	if(contentScaleChanged)
		applyContentScaleFactor ();

	if(IGraphicsLayer* layer = window.getGraphicsLayer ())
		layer->flush ();

	wl_surface_commit (getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESWindowRenderTarget::onFrameCallback ()
{
	Vector<LinuxWindow*> subSurfaces;
	linuxWindow->getSubSurfaces (subSurfaces);
	for(LinuxWindow* subSurface : subSurfaces)
	{
		OpenGLESWindowRenderTarget* subSurfaceRenderTarget = static_cast<OpenGLESWindowRenderTarget*> (subSurface->getRenderTarget ());
		if(!subSurface->isConfigured ())
		{
			subSurface->isConfigured (true);
			subSurfaceRenderTarget->onSize ();
			subSurfaceRenderTarget->applySize ();
			linuxWindow->setUserSize (linuxWindow->getSize ()); //< recalculate window bounds including the new subsurface
		}
		subSurfaceRenderTarget->onFrameCallback ();
	}

	if(invalidateRegion.getRects ().isEmpty () && updateRegion.getRects ().isEmpty ())
	{
		wl_surface_commit (getWaylandSurface ());
		return true;
	}

	onPresent ();
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{
	invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* OpenGLESWindowRenderTarget::getUpdateRegion ()
{
	return &updateRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* OpenGLESWindowRenderTarget::getInvalidateRegion ()
{
	return &invalidateRegion;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	addOpenGLES3DSurface (surface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	removeOpenGLES3DSurface (surface);
}

//************************************************************************************************
// OpenGLESLayerRenderTarget
//************************************************************************************************

OpenGLESLayerRenderTarget::OpenGLESLayerRenderTarget (wl_surface* surface, NativeGraphicsLayer& layer)
: LinuxLayerRenderTarget (surface, layer),
  sizeChanged (false)
{
	setWaylandSurface (surface);
	setContentScaleFactor (1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool OpenGLESLayerRenderTarget::initialize ()
{
	return LinuxOpenGLESRenderTarget::initialize (waylandSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::clear ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float OpenGLESLayerRenderTarget::getContentScaleFactor () const
{
	return contentScaleFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* OpenGLESLayerRenderTarget::getCanvas ()
{
	return LinuxOpenGLESRenderTarget::getSkiaCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::setContentScaleFactor (float factor)
{
	if(contentScaleFactor != factor)
	{
		onContentScaleFactorChanged (factor);
		contentScaleFactor = factor;
		onSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::resize (RectRef newSize)
{
	if(size == newSize)
		return;
	size = newSize;
	onSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef OpenGLESLayerRenderTarget::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::onSize ()
{
	pixelSize = PixelPoint (size.getSize (), contentScaleFactor);

	if(eglSurface == nullptr)
	{
		applySize ();
		initialize ();
	}

	sizeChanged = true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::applySize ()
{
	surface = nullptr;
	surfaceExtent = pixelSize;
	if(eglWindow)
		wl_egl_window_resize (eglWindow, surfaceExtent.x, surfaceExtent.y, 0, 0);
	clear ();
	sizeChanged = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::onRender ()
{
	makeCurrent ();
	OpenGLESClient::instance ().getGPUContext ()->resetContext (kAll_GrBackendState);

	if(sizeChanged)
		applySize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void OpenGLESLayerRenderTarget::onPresent ()
{
	if(surface)
	{
		if(flushSurface ())
			presentFrame ();
	}

	if(contentScaleChanged)
		applyContentScaleFactor ();

	wl_surface_commit (getWaylandSurface ());
}
