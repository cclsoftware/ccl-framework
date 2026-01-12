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
// Filename    : ccl/platform/cocoa/gui/transparentwindow.ios.mm
// Description : Transparent Window
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/gui/transparentwindow.ios.h"
#include "ccl/platform/cocoa/quartz/device.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"

#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/graphics/imaging/offscreen.h"

#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/windows/window.h"

namespace CCL {

//************************************************************************************************
// IOSTransparentWindow
//************************************************************************************************

class IOSTransparentWindow: public TransparentWindow
{
public:
	IOSTransparentWindow (Window* parentWindow, int options, StringRef title);
	~IOSTransparentWindow ();

	PROPERTY_POINTER (CCL_ISOLATED (TransparentWindowView), transparentView, TransparentView)

    void suspend (bool state);

	// TransparentWindow
	void show ();
	void hide ();
	bool isVisible () const;
	void update (RectRef size, Bitmap& bitmap, PointRef offset = Point (), float opacity = 1.f);
	void move (PointRef position);
};

} // namespace CCL

using namespace CCL;
using namespace MacOS;

//************************************************************************************************
// TransparentWindowView
//************************************************************************************************

@implementation CCL_ISOLATED (TransparentWindowView)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (id)initWithFrame:(CGRect)frameRect transparentWindow:(CCL::IOSTransparentWindow*)window
{
	self = [super initWithFrame:frameRect];
    [self setOpaque:NO];

	self->transparentWindow = window;
	return self;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)drawRect:(CGRect)dirtyRect
{
	Bitmap* bitmap = transparentWindow ? transparentWindow->getSavedBitmap () : nullptr;
	if(!bitmap)
		return;

	@try
	{
		CCL_PRINTF ("transparentWindow draw  %4d %4d %4d %4d\n", (int)dirtyRect.origin.x, (int)dirtyRect.origin.y, (int)dirtyRect.size.width, (int)dirtyRect.size.height)

		// draw the saved offscreen bitmap
		Window* parent = transparentWindow->getParentWindow ();
		ASSERT (parent)
		if(parent)
		{
			QuartzIOSWindowRenderTarget renderTarget (*parent);
			QuartzScopedGraphicsDevice nativeDevice (renderTarget, static_cast<Unknown&> (renderTarget));
			GraphicsDevice device;
			device.setNativeDevice (&nativeDevice);
			bitmap->draw (device, CCL::Point ());
		}
	}
	@catch(NSException* e)
	{
		CCL_WARN ("NSException in drawRect\n", 0)
	}
}
@end

//************************************************************************************************
// TransparentWindow
//************************************************************************************************

TransparentWindow* TransparentWindow::create (Window* parentWindow, int options, StringRef title)
{
	return NEW IOSTransparentWindow (parentWindow, options, title);
}

//************************************************************************************************
// IOSTransparentWindow
//************************************************************************************************

IOSTransparentWindow::IOSTransparentWindow (Window* parentWindow, int options, StringRef title)
: TransparentWindow (parentWindow, options, title),
  transparentView (nil)
{
    CCL_PRINTLN ("IOSTransparentWindow ctor")
    UIViewController* viewController = parentWindow ? (UIViewController*)parentWindow->getSystemWindow () : nil;
    if(viewController && viewController.view)
    {
        CGRect bounds = CGRectMake (0, 0, 100, 100);
        transparentView = [[CCL_ISOLATED (TransparentWindowView) alloc] initWithFrame:bounds transparentWindow:this];
        [viewController.view addSubview:transparentView];
		[transparentView setNeedsDisplay];
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IOSTransparentWindow::~IOSTransparentWindow ()
{
    CCL_PRINTLN ("~IOSTransparentWindow" )
    if(transparentView)
    {
        [transparentView removeFromSuperview];
        [transparentView release];
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSTransparentWindow::show ()
{
    CCL_PRINTLN ("TransparentWindow::show")
    if(transparentView)
        [transparentView setHidden:NO];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSTransparentWindow::hide ()
{
    CCL_PRINTLN ("TransparentWindow::hide")
    if(transparentView)
        [transparentView setHidden:YES];
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool IOSTransparentWindow::isVisible () const
{
    return transparentView ? ![transparentView isHidden] : false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSTransparentWindow::update (RectRef size, Bitmap& bitmap, PointRef offset, float opacity)
{
    CCL_PRINTF ("TransparentWindow::update: pos (%d, %d) size (%d, %d) offset (%d, %d)\n",
                size.left, size.top, size.getWidth (), size.getHeight (), offset.x, offset.y)

	// copy bitmap into offscreen
	AutoPtr<Offscreen> offscreen = NEW Offscreen (size.getWidth (), size.getHeight (), IBitmap::kRGBAlpha);
	{
		Rect dst (0, 0, size.getWidth (), size.getHeight ());
		Rect src (dst);
		src.offset (offset);
		BitmapGraphicsDevice device (offscreen);
		device.drawImage (&bitmap, src, dst);
	}
	setSavedBitmap (offscreen);

    if(transparentView)
    {
        // move/size the view
        CGRect frame;
        MacOS::toCGRect (frame, size);
        [transparentView setFrame:frame];

        [transparentView setNeedsDisplay];
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void IOSTransparentWindow::move (PointRef position)
{
    CCL_PRINTF ("TransparentWindow::move: x = %d y = %d\n", position.x, position.y)
    if(transparentView)
    {
        // move/size the view
        CGRect frame = [transparentView frame];
        frame.origin.x = position.x;
        frame.origin.y = position.y;

        [transparentView setFrame:frame];
    }
}
