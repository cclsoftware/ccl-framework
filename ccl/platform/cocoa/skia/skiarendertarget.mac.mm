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
// Filename    : ccl/platform/cocoa/skia/skiarendertarget.mac.mm
// Description : Skia Render Target for Mac using Metal
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/skia/skiarendertarget.mac.h"

#include "ccl/platform/cocoa/gui/window.mac.h"
#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/gui/platformwindow.mac.h"
#include "ccl/platform/cocoa/metal/metalclient.h"
#include "ccl/platform/cocoa/cclcocoa.h"

#include "ccl/public/base/ccldefpush.h"

#include <QuartzCore/CAMetalLayer.h>
#include <MetalKit/MetalKit.h>

using namespace CCL;

//************************************************************************************************
// SkiaWindowRenderTarget
//************************************************************************************************

SkiaWindowRenderTarget* SkiaWindowRenderTarget::create (Window& window)
{
	SkiaWindowRenderTarget* result = nullptr;

	if(MetalClient::instance ().isSupported ())
		result = NEW MetalMacWindowRenderTarget (window);

	return result;
}

//************************************************************************************************
// MetalMacWindowRenderTarget
//************************************************************************************************

MetalMacWindowRenderTarget::MetalMacWindowRenderTarget (Window& window)
: MetalWindowRenderTarget (window),
  hostView (nullptr),
  sizeObserver (nil),
  scaleObserver (nil),
  siblingViewCount (0)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

MetalMacWindowRenderTarget::~MetalMacWindowRenderTarget ()
{
	if(sizeObserver)
		[[NSNotificationCenter defaultCenter] removeObserver:sizeObserver];
	if(scaleObserver)
		[[NSNotificationCenter defaultCenter] removeObserver:scaleObserver];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalMacWindowRenderTarget::initialize ()
{
	ASSERT(!metalLayer)
	if(metalLayer)
		return;
	
	size = PixelPoint (Point (window.getWidth (), window.getHeight ()), window.getContentScaleFactor ());
	
	metalLayer = [[CAMetalLayer layer] retain];
	metalLayer.backgroundColor = [[NSColor colorWithRed:0.0 green:0.0 blue:0.0 alpha:0.0] CGColor];
	metalLayer.framebufferOnly = NO;
	metalLayer.device = metalDevice;
	metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	metalLayer.opaque = NO;
	delegate = [[CCL_ISOLATED (LayerDelegate) alloc] init];
	metalLayer.delegate = delegate;
	metalLayer.bounds = CGRectMake (0, 0, window.getWidth (), window.getHeight ());
	metalLayer.drawableSize = CGSizeMake (size.x, size.y);
	metalLayer.contentsScale = window.getContentScaleFactor ();
	
	OSXWindow* osxWindow = OSXWindow::cast (&window);
	MacOS::NativeView* view = osxWindow->getNativeView ();
	ASSERT (view)
	if(!view)
		return;
	view->setLayer (metalLayer);
		
	hostView = view->getView ();
	ASSERT (hostView)
	if(!hostView)
		return;
	
	__block MetalMacWindowRenderTarget* target = this;
	sizeObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSViewFrameDidChangeNotification object:hostView queue:nil usingBlock:^(NSNotification* note)
		{
			target->onSize ();
		}];
	scaleObserver = [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidChangeBackingPropertiesNotification object:[hostView window] queue:nil usingBlock:^(NSNotification* note)
		{
			target->onSize ();
			
		}];

	MetalWindowRenderTarget::initialize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalMacWindowRenderTarget::onSize ()
{
    // don't manipulate window size (origin) when resized via PlatformPanel::expandIfNeeded
    bool mustUpdateSize = true;
    auto platformWindow = toNSWindow (&window);
    if(platformWindow && [platformWindow isKindOfClass:[CCL_ISOLATED (PlatformPanel) class]])
        if([((CCL_ISOLATED (PlatformPanel)*)platformWindow) isInResize])
            mustUpdateSize = false;

    if(mustUpdateSize)
        window.updateSize ();

	MetalWindowRenderTarget::onSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalMacWindowRenderTarget::onRender ()
{
	// according to Apple documentation, clipping of overlapping sibling views is not handled at all by Cocoa -> so we need to take care of this
	NSView* parentView = [hostView superview];
	if([parentView isKindOfClass:[CCL_ISOLATED (ContentView) class]])
	{
		unsigned long nSiblings = [[parentView subviews] count] - 1;
		if(nSiblings != siblingViewCount)
		{
			siblingViewCount = nSiblings;
			invalidateRegion.addRect (Rect (0, 0, window.getWidth (), window.getHeight ()));
		}
	}

	if(!invalidateRegion.getRects ().isEmpty () && siblingViewCount > 0)
	{
		unsigned long nSubviews = siblingViewCount + 1;
		// find own view
		unsigned long ownIndex = 0;
		for(unsigned long i = 0; i < nSubviews; i++)
			if([[parentView subviews] objectAtIndex:i] == hostView)
			{
				ownIndex = i;
				break;
			}

		if(ownIndex + 1 < nSubviews) // true if siblings lie above own view (have higher index)
		{
			SkCanvas* canvas = getCanvas ();
			canvas->save ();
			// determine intersections and clip
			for(unsigned long i = ownIndex + 1; i < nSubviews; i++)
			{
				NSView* subView = [[parentView subviews] objectAtIndex:i];
				NSRect siblingFrame = [subView frame];
				NSRect intersection = NSIntersectionRect ([hostView frame], siblingFrame);
				if(!NSIsEmptyRect (intersection) && [subView isOpaque])
				{
					NSRect clip = NSOffsetRect (intersection, [hostView frame].origin.x, [hostView frame].origin.x);

					SkRect clipRect = SkRect::MakeXYWH (static_cast<float> (clip.origin.x), static_cast<float> (clip.origin.y), static_cast<float> (clip.size.width), static_cast<float> (clip.size.height));
					SkPaint paint;
					paint.setStyle (SkPaint::kFill_Style);
					paint.setBlendMode (SkBlendMode::kClear);
					canvas->drawRect (clipRect, paint);
					canvas->clipRect (clipRect, SkClipOp::kDifference);
				}
			}
			MetalWindowRenderTarget::onRender ();
			canvas->restore ();
			return;
		}
	}
	MetalWindowRenderTarget::onRender ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalMacWindowRenderTarget::addMetal3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		[hostView addSubview:metal3DSurface->getView ()];
		MetalWindowRenderTarget::addMetal3DSurface (metal3DSurface);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void MetalMacWindowRenderTarget::removeMetal3DSurface (Native3DSurface* surface)
{
	if(Metal3DSurface* metal3DSurface = ccl_cast<Metal3DSurface> (surface))
	{
		MetalWindowRenderTarget::removeMetal3DSurface (metal3DSurface);
		[metal3DSurface->getView () removeFromSuperview];
	}
}
