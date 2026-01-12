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
// Filename    : ccl/platform/cocoa/skia/skialayer.cocoa.mm
// Description : CoreAnimation Graphics Layer for Skia content
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/skia/skialayer.cocoa.h"
#include "ccl/platform/cocoa/gui/cagraphicslayer.cocoa.h"

#include "ccl/platform/shared/skia/skiadevice.h"

#include "ccl/platform/cocoa/skia/skiarendertarget.cocoa.h"
#include "ccl/platform/cocoa/cclcocoa.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/gui/system/animation.h"

#include "ccl/gui/windows/window.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/graphicshelper.h"
#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/gui/graphics/imaging/image.h"

#if CCL_PLATFORM_MAC
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/gui/platformwindow.mac.h"
#define COLORTYPE NSColor
#elif CCL_PLATFORM_IOS
#include "ccl/platform/cocoa/gui/window.ios.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#define COLORTYPE UIColor
#endif

#include "ccl/public/base/ccldefpush.h"

#import <QuartzCore/CAMetalLayer.h>
#include <QuartzCore/CAAnimation.h>
#include <QuartzCore/CATransaction.h>
#include <QuartzCore/CAMediaTimingFunction.h>

#define HIGHLIGHT_LAYERS 0 && DEBUG

namespace CCL {
namespace MacOS {

//************************************************************************************************
// CocoaSkiaLayer
//************************************************************************************************

class CocoaSkiaLayer : public CoreAnimationLayer
{
public:
	DECLARE_CLASS (CocoaSkiaLayer, CoreAnimationLayer)

	CocoaSkiaLayer ();
	~CocoaSkiaLayer ();
	
	// CoreAnimationLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	tresult CCL_API setContent (IUnknown* content) override;
	void CCL_API setSize (Coord width, Coord height) override;
	void CCL_API setOffset (PointRef offset) override;
	void CCL_API setOffsetX (float offsetX) override;
	void CCL_API setOffsetY (float offsetY) override;
	void CCL_API setContentScaleFactor (float factor) override;
	void CCL_API setUpdateNeeded () override;
	void CCL_API setUpdateNeeded (RectRef rect) override;
	void CCL_API suspendTiling (tbool suspend, const Rect* visibleRect) override { ; }
	tresult CCL_API flush () override;
	
protected:
	SharedPtr<IUnknown> content;
	Rect contentRect;
	Rect dirtyRect;
	Rect currentSize;
	bool contentNeedsFlush;

	AutoPtr<MetalLayerRenderTarget> renderTarget;
	float contentScaleFactor;

	void applySize ();
	void drawContent ();
};

//************************************************************************************************
// CocoaSkiaRootLayer
//************************************************************************************************

class CocoaSkiaRootLayer : public CocoaSkiaLayer
{
public:
	DECLARE_CLASS (CocoaSkiaRootLayer, CocoaSkiaLayer)

	~CocoaSkiaRootLayer ();

	// CocoaSkiaLayer
	tresult CCL_API construct (IUnknown* content, RectRef bounds = Rect (), int mode = 0, float contentScaleFactor = 1.f) override;
	void CCL_API setSize (Coord width, Coord height) override;
	tresult CCL_API flush () override;

protected:
	#if CCL_PLATFORM_MAC
	NSObj<NSView> layerHost;
	#endif
};

}
}

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// CocoaSkiaLayerFactory
//************************************************************************************************

IGraphicsLayer* CocoaSkiaLayerFactory::createLayer (UIDRef classID)
{
	if(classID == ClassID::RootLayer)
		return NEW CocoaSkiaRootLayer;
	if(classID == ClassID::GraphicsLayer)
		return NEW CocoaSkiaLayer;
	if(classID == ClassID::TiledLayer)
		return NEW CocoaSkiaLayer; // not yet implemented
	return nullptr;
}

//************************************************************************************************
// CocoaSkiaLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CocoaSkiaLayer, CoreAnimationLayer)

CocoaSkiaLayer::CocoaSkiaLayer ()
: contentNeedsFlush (false),
  contentScaleFactor (1.f)
{
	setDeferredRemoval (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSkiaLayer::~CocoaSkiaLayer ()
{
	if(contentNeedsFlush)
		MetalUpdater::instance ().removeLayer (this);

	CCL_PRINTF ("Destroy layer:%p\n", this)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaLayer::construct (IUnknown* _content, RectRef bounds, int mode, float _contentScaleFactor)
{
	CGSize pixelSize = CGSizeMake (bounds.getWidth () * _contentScaleFactor, bounds.getHeight () * _contentScaleFactor);
	CCL_PRINTF ("Construct layer:%p (%f px x %f px)\n", this, pixelSize.width, pixelSize.height)
	
	ASSERT (!content.isValid ()) // must be called only once
	if(content.isValid ())
		return kResultUnexpected;

	contentScaleFactor = _contentScaleFactor;
	contentRect.setSize (bounds.getSize ());
		
	CAMetalLayer* metalLayer = [[CAMetalLayer layer] retain];
	
	metalLayer.anchorPoint = CGPointMake (0.f, 0.f);
	metalLayer.contentsScale = contentScaleFactor;
	CGRect frame;
	toCGRect (frame, bounds);
	if(frame.size.width <= 0)
		frame.size.width = 1;
	if(frame.size.height <= 0)
		frame.size.height = 1;
		
	metalLayer.bounds = CGRectMake (0, 0, frame.size.width, frame.size.height);
	metalLayer.position = frame.origin;
	metalLayer.masksToBounds = (mode & kClipToBounds) ? YES : NO; // clip sublayers
	metalLayer.opaque = (mode & kIgnoreAlpha) ? YES : NO;
	metalLayer.drawableSize = pixelSize;
	metalLayer.presentsWithTransaction = YES;
	#if HIGHLIGHT_LAYERS
	[metalLayer setBorderColor: [[COLORTYPE blueColor] CGColor]];
	[metalLayer setBorderWidth:1.0];
	if(!(mode & kClipToBounds))
		metalLayer.backgroundColor = [[NSColor greenColor] CGColor];
	#endif
	
	renderTarget = NEW MetalLayerRenderTarget (metalLayer, contentScaleFactor);
	if(!renderTarget->checkSize (pixelSize))
	{
		ASSERT (false)
		return kResultFailed;
	}
	nativeLayer = metalLayer;
	
	setContent (_content);
	
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaLayer::setContent (IUnknown* _content)
{
	CCL_PRINTLN ("CocoaSkiaLayer::setContent")
	if(contentNeedsFlush)
		MetalUpdater::instance ().removeLayer (this);
		
	content = _content;
	UnknownPtr<IGraphicsLayerContent> layerContent (content);
	contentNeedsFlush = layerContent != nullptr;
	if(contentNeedsFlush)
	{
		MetalUpdater::instance ().addLayer (this);
		setUpdateNeeded ();
	}
	else
	{
		dirtyRect = contentRect;
		drawContent ();
	}

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaSkiaLayer::applySize ()
{
	currentSize = contentRect;
	nativeLayer.bounds = CGRectMake (0, 0, contentRect.getWidth (), contentRect.getHeight ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CocoaSkiaLayer::drawContent ()
{
	if(currentSize != contentRect)
		applySize ();

	if(dirtyRect.isEmpty ())
		return;
	
	if(!renderTarget)
		return;

	if(contentRect.getWidth () * contentRect.getHeight () <= 0)
		return;
		
	{
		SkiaScopedGraphicsDevice nativeDevice (*renderTarget, *renderTarget->asUnknown ());
		{
			GraphicsDevice graphics;
			graphics.setNativeDevice (&nativeDevice);
			Rect clipRect (dirtyRect);
			graphics.clearRect (clipRect);
			graphics.addClip (clipRect);

			Point delta;
			
			if(Bitmap* bitmap = unknown_cast<Bitmap> (content))
			{
				Rect dst (0, 0, contentRect.getWidth (), contentRect.getHeight ());
				graphics.drawImage (bitmap, contentRect, dst);
			}
			else
			{
				UnknownPtr<IGraphicsLayerContent> layerContent (content);
				if(layerContent)
				{
					IGraphicsLayerContent::LayerHint layerHint = layerContent->getLayerHint ();
					if(layerHint == kGraphicsContentEmpty)
						return;
					layerContent->drawLayer (graphics, UpdateRgn (dirtyRect), delta);
				}
			}
		}
	}
	
	renderTarget->onRender ();
	dirtyRect.setReallyEmpty ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setSize (Coord width, Coord height)
{
	if(width == contentRect.getWidth () && height == contentRect.getHeight ())
		return;

	CGSize pixelSize = CGSizeMake (width * contentScaleFactor, height * contentScaleFactor);
	if(!renderTarget->checkSize (pixelSize))
	{
		ASSERT (false)
		return;
	}

	contentRect.setSize (Point (width, height));
	CAMetalLayer* metalLayer = static_cast<CAMetalLayer*> (nativeLayer);
	metalLayer.drawableSize = pixelSize;
	renderTarget->onSize ();
	setUpdateNeeded ();
	CCL_PRINTF ("layer \"%s\" ", name.str ()) PRINT_CGRECT ("setSize ", nativeLayer.frame)
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setContentScaleFactor (float factor)
{
	contentScaleFactor = factor;
	if(nativeLayer)
		if(factor != getContentScaleFactor ())
		{
			if(renderTarget)
			{
				renderTarget->setContentScaleFactor (factor);
				renderTarget->onSize ();
			}
			dirtyRect.join (contentRect);
			flush ();
		}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setUpdateNeeded ()
{
	setUpdateNeeded (contentRect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setUpdateNeeded (RectRef rect)
{
	CCL_PRINTF ("CocoaSkiaLayer::setUpdateNeeded (%d %d) of (%d %d)\n", rect.getWidth (), rect.getHeight (), contentRect.getWidth (), contentRect.getHeight ())
	if(!unknown_cast<Image> (content))
		dirtyRect.join (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaLayer::flush ()
{
	drawContent ();

	removePendingSublayersFromParent ();

	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setOffset (PointRef offset)
{
	CoreAnimationLayer::setOffset (offset);
	flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setOffsetX (float offsetX)
{
	CoreAnimationLayer::setOffsetX (offsetX);
	flush ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaLayer::setOffsetY (float offsetY)
{
	CoreAnimationLayer::setOffsetY (offsetY);
	flush ();
}

//************************************************************************************************
// CocoaSkiaRootLayer
//************************************************************************************************

DEFINE_CLASS_HIDDEN (CocoaSkiaRootLayer, CocoaSkiaLayer)

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaSkiaRootLayer::~CocoaSkiaRootLayer ()
{
	#if CCL_PLATFORM_MAC
	MetalUpdater::instance ().removeLayer (this);
	if(layerHost)
		[layerHost removeFromSuperview];
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaRootLayer::construct (IUnknown* _content, RectRef bounds, int mode, float contentScaleFactor)
{
	Window* window = unknown_cast<Window> (_content);
	// do not assign _content to content, this object is owned by the window
	ASSERT (window)
	if(!window)
		return kResultInvalidArgument;

	#if CCL_PLATFORM_MAC
	OSXWindow* osxWindow = OSXWindow::cast (window);
	ASSERT (osxWindow)
	RectRef windowSize = osxWindow->getSize ();
	layerHost = [[CCL_ISOLATED (FlippedView) alloc] initWithFrame:CGRectMake (0, 0, windowSize.getWidth (), windowSize.getHeight ()) childWindow:osxWindow->isChildWindow ()];
	[layerHost setWantsLayer:YES];
	[layerHost setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
	NativeView layerView (layerHost);
	osxWindow->embed (&layerView);
	nativeLayer = [layerHost layer];
	MetalUpdater::instance ().addLayer (this);

	#else
	IOSWindow* iosWindow = IOSWindow::cast (window);
	NativeView* view = iosWindow->getNativeView ();
	nativeLayer = view->getLayer ();
	#endif
	[nativeLayer retain];
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API CocoaSkiaRootLayer::setSize (Coord width, Coord height)
{
	// root layer is automaticaly sized
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API CocoaSkiaRootLayer::flush ()
{
	removePendingSublayersFromParent ();

	[CATransaction flush];
	return kResultTrue;
}
