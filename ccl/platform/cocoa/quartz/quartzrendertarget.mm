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
// Filename    : ccl/platform/cocoa/quartz/quartzrendertarget.mm
// Description : Quartz Engine
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/quartz/quartzrendertarget.h"

#include "ccl/gui/windows/window.h"
#include "ccl/public/gui/graphics/dpiscale.h"
#include "ccl/platform/cocoa/cclcocoa.h"

#if CCL_PLATFORM_MAC

#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/gui/platformwindow.mac.h"
#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/quartz/nshelper.h"
#include "ccl/platform/cocoa/quartz/quartzbitmap.h"

#elif CCL_PLATFORM_IOS

#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/platform/cocoa/gui/window.ios.h"

#endif

#include "ccl/public/base/ccldefpush.h"
#include <MetalKit/MetalKit.h>

namespace CCL {
namespace MacOS {

//************************************************************************************************
// QuartzRenderTarget
//************************************************************************************************

QuartzRenderTarget::QuartzRenderTarget (CGContextRef _context)
: context (_context)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzRenderTarget::addMetal3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
		surfaces.add (metal3DSurface);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzRenderTarget::removeMetal3DSurface (Native3DSurface* surface)
{
	surfaces.remove (ccl_cast<Metal3DSurface> (surface));
}

//************************************************************************************************
// QuartzLayerRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (QuartzLayerRenderTarget, Object)

QuartzLayerRenderTarget::QuartzLayerRenderTarget (CGContextRef _context, float _contentScaleFactor)
: context (_context),
  contentScaleFactor (_contentScaleFactor)
{}

//************************************************************************************************
// NativeViewUpdateRegion
//************************************************************************************************

void CCL_API NativeViewUpdateRegion::addRect (RectRef rect)
{
	MutableRegion::addRect (rect);
	CGRect updateRect = CGRectMake (rect.left, rect.top, rect.getWidth (), rect.getHeight ());
	CCL_PRINTF ("NativeViewUpdateRegion::addRect  %4d %4d %4d %4d\n", (int)updateRect.origin.x, (int)updateRect.origin.y, (int)updateRect.size.width, (int)updateRect.size.height)
	
	if(nativeView)
		[nativeView->getView () setNeedsDisplayInRect:updateRect];
}

#if CCL_PLATFORM_MAC

//************************************************************************************************
// QuartzOSXWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (QuartzOSXWindowRenderTarget, NativeWindowRenderTarget)

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzOSXWindowRenderTarget::QuartzOSXWindowRenderTarget (Window& _window)
: NativeWindowRenderTarget (_window),
  nativeView (nullptr),
  didLockFocus (false)
{
	OSXWindow* osxWindow = OSXWindow::cast (&_window);
	ASSERT (osxWindow)
	nativeView = osxWindow->getNativeView ();
	ASSERT (nativeView)
    invalidateRegion.setNativeView (nativeView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzOSXWindowRenderTarget::~QuartzOSXWindowRenderTarget ()
{
    releaseContext ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::createContext ()
{
    if(!nativeView)
        return;

    ASSERT (context == nullptr)
    context = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    if(!context)
    {
		// lockFocus / unlockFocus (for direct draw) is deprecated
		#pragma clang diagnostic push
		#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        [nativeView->getView () lockFocus];
        #pragma clang diagnostic pop
        didLockFocus = true;
        context = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    }
    
    ASSERT (context)
	ASSERT ([NSGraphicsContext currentContextDrawingToScreen] == YES)
    CGContextSaveGState (context);

    CGFloat width = [nativeView->getView () bounds].size.width;
    CGFloat height = [nativeView->getView () bounds].size.height;
        
    NSArray* subViews = [nativeView->getView () subviews];
    for(NSView* subView in subViews)
    {
        if(![subView isKindOfClass:[CCL_ISOLATED (FlippedView) class]])
        {
            CGRect rects[4];
            rects[0] = CGRectMake (0, 0, width, [subView frame].origin.y); // upper
            rects[1] = CGRectMake (0, [subView frame].origin.y, [subView frame].origin.x, height - [subView frame].origin.y); // left
            rects[2] = CGRectMake ([subView frame].origin.x + [subView frame].size.width, [subView frame].origin.y, width - ([subView frame].origin.x + [subView frame].size.width), height - [subView frame].origin.y); // right
            rects[3] = CGRectMake ([subView frame].origin.x, [subView frame].origin.y + [subView frame].size.height, [subView frame].size.width, height - ([subView frame].origin.y + [subView frame].size.height)); // lower
            CGContextClipToRects (context, rects, 4);
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::releaseContext ()
{
    if(!nativeView)
        return;
    
    if(context)
    {
        if(didLockFocus)
        {
			#pragma clang diagnostic push
			#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            [nativeView->getView () unlockFocus];
            #pragma clang diagnostic pop
             didLockFocus = false;
        }
        
        CGContextRestoreGState (context);
        CGContextSynchronize (context);
    
        context = nullptr;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuartzOSXWindowRenderTarget::shouldCollectUpdates ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::onRender ()
{
    if(!nativeView)
        return;
	
	[[nativeView->getView () window] displayIfNeeded];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{
    NSSize nsOffset = {static_cast<CGFloat>(delta.x), static_cast<CGFloat>(delta.y)};
    NSRect nsRect;
    MacOS::toNSRect (nsRect, rect);
	// scrollRect for NSView is deprecated, should use NSScrollView instead
	#pragma clang diagnostic push
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    [nativeView->getView () scrollRect:nsRect by:nsOffset];
    #pragma clang diagnostic pop
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float QuartzOSXWindowRenderTarget::getContentScaleFactor () const
{
    return window.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGContextRef QuartzOSXWindowRenderTarget::getContext ()
{
    if(!context)
        createContext ();
    return context;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::flush ()
{
    releaseContext ();
    
    // invalidate one pixel to trigger actual drawing of the context
    [nativeView->getView () setNeedsDisplayInRect:NSMakeRect (0, 0, 1, 1)];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* QuartzOSXWindowRenderTarget::getUpdateRegion ()
{
    return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		[nativeView->getView () addSubview:metal3DSurface->getView ()];
		QuartzRenderTarget::addMetal3DSurface (metal3DSurface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzOSXWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		QuartzRenderTarget::removeMetal3DSurface (metal3DSurface);
		[metal3DSurface->getView () removeFromSuperview];
	}
}

#endif

#if CCL_PLATFORM_IOS
//************************************************************************************************
// QuartzIOSWindowRenderTarget
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (QuartzIOSWindowRenderTarget, NativeWindowRenderTarget)


//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzIOSWindowRenderTarget::QuartzIOSWindowRenderTarget (Window& window)
: NativeWindowRenderTarget (window)
{
	IOSWindow* iosWindow = IOSWindow::cast (&window);
	ASSERT (iosWindow)
	nativeView = iosWindow->getNativeView ();
	ASSERT (nativeView)
	invalidateRegion.setNativeView (nativeView);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

QuartzIOSWindowRenderTarget::~QuartzIOSWindowRenderTarget ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool QuartzIOSWindowRenderTarget::shouldCollectUpdates ()
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IMutableRegion* QuartzIOSWindowRenderTarget::getUpdateRegion ()
{
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzIOSWindowRenderTarget::onRender ()
{
	if(!nativeView)
		return;

	RectRef rect = invalidateRegion.getBoundingBox ();
	[nativeView->getView () setNeedsDisplayInRect:CGRectMake (rect.left, rect.top, rect.getWidth (), rect.getHeight ())];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzIOSWindowRenderTarget::onSize ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzIOSWindowRenderTarget::onScroll (RectRef rect, PointRef delta)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

float QuartzIOSWindowRenderTarget::getContentScaleFactor () const
{
    return window.getContentScaleFactor ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

CGContextRef QuartzIOSWindowRenderTarget::getContext ()
{
	return UIGraphicsGetCurrentContext ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzIOSWindowRenderTarget::add3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		[nativeView->getView () addSubview:metal3DSurface->getView ()];
		QuartzRenderTarget::addMetal3DSurface (metal3DSurface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void QuartzIOSWindowRenderTarget::remove3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		QuartzRenderTarget::removeMetal3DSurface (metal3DSurface);
		[metal3DSurface->getView () removeFromSuperview];
	}
}
#endif

} // namespace MacOS
} // namespace CCL
