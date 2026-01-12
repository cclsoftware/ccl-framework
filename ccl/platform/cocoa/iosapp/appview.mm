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
// Filename    : ccl/platform/cocoa/iosapp/appview.mm
// Description : iOS View to CCL form bridge
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/cocoa/iosapp/appview.h"

#include "ccl/gui/windows/childwindow.h"
#include "ccl/gui/windows/windowmanager.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/gui/nativeview.ios.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/graphics/graphicsdevice.h"
#include "ccl/gui/theme/thememanager.h"

using namespace CCL;

//************************************************************************************************
// AppWindow
//************************************************************************************************

class AppWindow: public ChildWindow
{
public:
    AppWindow (RectRef size, CCL_ISOLATED (AppView)* appView, IView* contentView, IUnknown* controller)
	: ChildWindow (kWindowModeEmbedding, size)
	{
		setController (controller);
		
		if(View* view = unknown_cast<View> (contentView))
		{
			// content view must fill parent
			CCL::Rect size;
			getClientRect (size);
			view->setSize (size);
			
			view->setSizeMode (View::kAttachAll);
			addView (view);
		}
		addToDesktop ();
		
		// bypass background renderer, set black background instead
		style.setCommonStyle (Styles::kTransparent);
		[appView setBackgroundColor:[UIColor blackColor]];
	}
    
    void setSystemWindow (void* systemWindow)
    {
		static bool attachedOnce = false;
        handle = systemWindow;
		nativeView = NEW NativeView (static_cast<CCL_ISOLATED (ContentView)*> ([static_cast<UIViewController*>(handle) view]));

		renderTarget = Window::getRenderTarget ();
		
		if(isAttached () && !attachedOnce)
		{
			attached (nullptr);
			attachedOnce = true;
		}
    }
	
	// ChildWindow
	void onChildLimitsChanged (View* child) override
	{
		// supress deferred "checkSizeLimits" in Window::onChildLimitsChanged
		View::onChildLimitsChanged (child);
	}
	
	tbool close () override
	{
		if(handle == nullptr || isInCloseEvent () || isInDestroyEvent ())
			return false;
		
		setInCloseEvent (true);
		setInDestroyEvent (true);
		
		onDestroy ();
		release ();
		
		return true;
	}
	
	void setWindowSize (CCL::Rect& newSize) override
	{
		// app window not resizable on iOS
		MacOS::fromCGRect (newSize, [[(UIViewController*)handle view] bounds]);
	}
};

//************************************************************************************************
// AppView
//************************************************************************************************

@implementation CCL_ISOLATED (AppView)

//////////////////////////////////////////////////////////////////////////////////////////////////

- (IView*)createApplicationView:(IApplication*)application;
{
	if(window != nullptr)
		return nullptr;
	
    CGRect size = [self frame];
    CCL::Rect rect;
    MacOS::fromCGRect (rect, size);
		
    IView* appView = WindowManager::instance ().createApplicationView (rect);

	[self setWindow: NEW AppWindow (rect, self, appView, application)];
	((AppWindow*)window)->setSystemWindow ([nativeWindow rootViewController]);
	
	
	return appView;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)willMoveToWindow:(UIWindow *)uiWindow
{
	nativeWindow = uiWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

- (void)setSize:(CCL::Rect&)size
{
	if(window)
	{
		Coord w = size.getWidth ();
		Coord h = size.getHeight ();
		SizeLimit limits (w, h, w, h);
		window->setSizeLimits (limits);
		window->setSize (size);
	}
}

@end
