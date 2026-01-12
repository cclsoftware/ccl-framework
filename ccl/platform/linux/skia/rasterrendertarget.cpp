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
// Filename    : ccl/platform/linux/skia/rasterrendertarget.cpp
// Description : Skia Render Target for Linux using Software Rendering
//
//************************************************************************************************

#include "ccl/platform/linux/skia/rasterrendertarget.h"
#include "ccl/platform/linux/gui/window.linux.h"

using namespace CCL;
using namespace Linux;

//************************************************************************************************
// RasterWindowRenderTarget
//************************************************************************************************

RasterRenderTarget::RasterRenderTarget ()
: currentBuffer (-1),
  lastSurface (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

sk_sp<SkSurface> RasterRenderTarget::getSurface (PointRef size)
{
	sk_sp<SkSurface> surface = nullptr;
	for(int i = 0; i < ARRAY_COUNT (buffers); i++)
	{
		if(!buffers[i].ready ())
			continue;
		
		SkColorType colorType = kBGRA_8888_SkColorType;
		const SkImageInfo& imageInfo = SkImageInfo::Make (size.x, size.y, colorType, kPremul_SkAlphaType);
		SkSurfaceProps props;
		
		buffers[i].resize (size, int (imageInfo.minRowBytes ()));
			
		surface = SkSurfaces::WrapPixels (imageInfo, buffers[i].getData (), imageInfo.minRowBytes (), &props);
			surface->getCanvas ()->scale (getScaleFactor (), getScaleFactor ());
			
		if(lastSurface && currentBuffer >= 0)
			surface->getCanvas ()->writePixels (imageInfo, buffers[currentBuffer].getData (), imageInfo.minRowBytes (), 0, 0);
		else
			surface->getCanvas ()->clear (SkColorSetARGB (0, 0, 0, 0));
			
		currentBuffer = i;
		
		break;
	}
	return surface;
}

//************************************************************************************************
// RasterWindowRenderTarget
//************************************************************************************************

RasterWindowRenderTarget::RasterWindowRenderTarget (Window& window)
: SkiaWindowRenderTarget (window)
{
	linuxWindow = LinuxWindow::cast (&window);
	setWaylandSurface (linuxWindow->getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* RasterWindowRenderTarget::getCanvas ()
{
	if(surface == nullptr)
		surface = getSurface (size);
	return SkiaWindowRenderTarget::getCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RasterWindowRenderTarget::onSize ()
{
	if(scaleFactor != window.getContentScaleFactor ())
		onContentScaleFactorChanged (window.getContentScaleFactor ());

    PixelPoint currentSize = size;
    size = PixelPoint (Point (ccl_max (window.getWidth (), 1), ccl_max (window.getHeight (), 1)), window.getContentScaleFactor ());
	if(size != currentSize)
	{
		lastSurface = nullptr;
		surface = nullptr;
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

void RasterWindowRenderTarget::onRender ()
{
	getCanvas ();
	if(surface && !invalidateRegion.getRects ().isEmpty ())
	{
		AutoPtr<NativeGraphicsDevice> nativeDevice = ensureGraphicsDevice (NativeGraphicsEngine::instance ().createWindowDevice (&window));
		AutoPtr<GraphicsDevice> graphicsDevice = NEW WindowGraphicsDevice (window, nativeDevice);
		window.setGraphicsDevice (graphicsDevice);
		
		for(int i = 0; i < invalidateRegion.getRects ().count (); i++)
		{
			const Rect& invalidateRect = invalidateRegion.getRects ().at (i);
			wl_surface_damage_buffer (getWaylandSurface (), invalidateRect.left, invalidateRect.top, invalidateRect.getWidth (), invalidateRect.getHeight ());
			
			graphicsDevice->saveState ();
			graphicsDevice->addClip (invalidateRect);
			if(window.getStyle ().isTranslucent () || window.getOpacity () < 1.f)
				graphicsDevice->clearRect (invalidateRect);
			
			window.setInDrawEvent (true);
			
			if(window.getOpacity () < 1.f)
			{
				SkPaint alpha;
				alpha.setAlphaf (window.getOpacity ());
				getCanvas ()->saveLayer (nullptr, &alpha);
			}
			
			window.draw (UpdateRgn (invalidateRect, &invalidateRegion));
			
			window.setInDrawEvent (false);
			graphicsDevice->restoreState ();
		}
		invalidateRegion.setEmpty ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RasterWindowRenderTarget::onPresent ()
{	
	onRender ();
	
	lastSurface = surface;
	surface = nullptr;
	
	if(currentBuffer >= 0)
		buffers[currentBuffer].attach (waylandSurface);
	
	if(contentScaleChanged)
		applyContentScaleFactor ();

	if(IGraphicsLayer* layer = window.getGraphicsLayer ())
		layer->flush ();

	wl_surface_commit (getWaylandSurface ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool RasterWindowRenderTarget::onFrameCallback ()
{
	Vector<LinuxWindow*> subSurfaces;
	linuxWindow->getSubSurfaces (subSurfaces);
	for(LinuxWindow* subSurface : subSurfaces)
	{
		if(!subSurface->isConfigured ())
		{
			subSurface->isConfigured (true);
			static_cast<RasterWindowRenderTarget*> (subSurface->getRenderTarget ())->onSize ();
			linuxWindow->setUserSize (linuxWindow->getSize ()); //< recalculate window bounds including the new subsurface 
		}
		static_cast<RasterWindowRenderTarget*> (subSurface->getRenderTarget ())->onFrameCallback ();
	}
	
	if(invalidateRegion.getRects ().isEmpty () && updateRegion.getRects ().isEmpty ())
	{
		wl_surface_commit (getWaylandSurface ());
		return true;
	}

	onPresent ();
	return true;
}

//************************************************************************************************
// RasterLayerRenderTarget
//************************************************************************************************

RasterLayerRenderTarget::RasterLayerRenderTarget (wl_surface* surface, NativeGraphicsLayer& layer)
: LinuxLayerRenderTarget (surface, layer)
{
	setWaylandSurface (surface);
	setContentScaleFactor (1.f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float RasterLayerRenderTarget::getContentScaleFactor () const
{
	return contentScaleFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

SkCanvas* RasterLayerRenderTarget::getCanvas ()
{
	if(surface == nullptr)
		surface = getSurface (size.getSize ());
	return SkiaRenderTarget::getCanvas ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RasterLayerRenderTarget::setContentScaleFactor (float factor)
{
	if(contentScaleFactor != factor)
	{
		onContentScaleFactorChanged (factor);
		contentScaleFactor = factor;
		onSize ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RasterLayerRenderTarget::resize (RectRef newSize)
{
	if(size == newSize)
		return;
	size = newSize;
	onSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

RectRef RasterLayerRenderTarget::getSize () const
{
	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RasterLayerRenderTarget::onSize ()
{
	lastSurface = nullptr;
	surface = nullptr;
	
	invalidateRegion.addRect (Rect (0, 0, size.getWidth (), size.getHeight ()));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void RasterLayerRenderTarget::onPresent ()
{
	wl_surface_damage_buffer (getWaylandSurface (), 0, 0, size.getWidth (), size.getHeight ());
	
	lastSurface = surface;
	surface = nullptr;
	
	if(currentBuffer >= 0)
		buffers[currentBuffer].attach (waylandSurface);
	
	if(contentScaleChanged)
		applyContentScaleFactor ();

	wl_surface_commit (getWaylandSurface ());
}
