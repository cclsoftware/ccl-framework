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
// Filename    : ccl/platform/cocoa/gui/desktop.cocoa.mm
// Description : Desktop Management
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/desktop.h"

#include "ccl/platform/cocoa/gui/nativeview.mac.h"
#include "ccl/platform/cocoa/quartz/cghelper.h"
#include "ccl/platform/cocoa/macutils.h"

#if CCL_PLATFORM_MAC
#include "ccl/platform/cocoa/quartz/nshelper.h"
#endif

#include "ccl/platform/cocoa/cclcocoa.h"

namespace CCL {

//************************************************************************************************
// CocoaDesktopManager
//************************************************************************************************

class CocoaDesktopManager: public DesktopManager
{
public:
	// 	DesktopManager
	IWindow* CCL_API findWindow (PointRef screenPos, int flags = 0);
	int CCL_API countMonitors () const;
	int CCL_API getMainMonitor () const;
	int CCL_API findMonitor (PointRef where, tbool defaultToPrimary) const;
	tbool CCL_API getMonitorSize (Rect& rect, int index, tbool useWorkArea) const;
	float CCL_API getMonitorScaleFactor (int index) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////

CocoaDesktopManager cocoaDesktop;
DesktopManager& Desktop = cocoaDesktop;

} // namespace CCL

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

#define ForEachWindow \
for(int layer = 0; layer < kNumWindowLayers; layer++) { ListForEach (windows[layer], Window*, w)

#define ForEachWindowReverse \
for(int layer = kNumWindowLayers-1 ; layer >= 0; layer--) { ListForEachReverse (windows[layer], Window*, w)

#define EndForWindow \
EndFor }

//************************************************************************************************
// CocoaDesktopManager
//************************************************************************************************

#if CCL_PLATFORM_MAC
static CGWindowID baseWindow = 0;
static CFObj<CFArrayRef> windowList;
static CFAbsoluteTime lastListUpdate = 0;
const CFTimeInterval updateInterval = 0.5;

static CFArrayRef getWindowsAbove (CGWindowID window)
{
	CFAbsoluteTime now = CFAbsoluteTimeGetCurrent ();
	bool mustUpdate = (window != baseWindow) || (now - lastListUpdate > updateInterval);
	if(mustUpdate)
	{
		windowList = CGWindowListCopyWindowInfo (kCGWindowListOptionOnScreenAboveWindow|kCGWindowListExcludeDesktopElements, window);
		lastListUpdate = now;
		baseWindow = window;
	}
	
	return windowList;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

IWindow* CCL_API CocoaDesktopManager::findWindow (PointRef screenPos, int flags)
{
	ForEachWindowReverse // topmost first
		Rect frameRect;
		w->getFrameSize (frameRect);
		CCL_PRINTF("Try window: width=%d height=%d layer=%d \n", frameRect.getWidth(), frameRect.getHeight(), w->getLayer ())
		if(frameRect.pointInside (screenPos) && !(w->isInCloseEvent () || w->isInDestroyEvent()))
		{	
			#if CCL_PLATFORM_MAC
			if(flags & IDesktop::kEnforceOcclusionCheck)
			{
				NSWindow* cocoaWindow = MacOS::toNSWindow (w);
				CFArrayRef windowList = getWindowsAbove ((CGWindowID)[cocoaWindow windowNumber]);
				
				if(windowList)
				{
					CGRect windowBounds;
					static CGWindowLevel dockWindowLevel = CGWindowLevelForKey (kCGDockWindowLevelKey);
					static CGWindowLevel cursorWindowLevel = CGWindowLevelForKey (kCGCursorWindowLevelKey);
					static CGWindowLevel accessibiltyWindowLevel = CGWindowLevelForKey (kCGAssistiveTechHighWindowLevelKey);
					CGPoint screenPoint = CGPointMake (screenPos.x, screenPos.y);
					for(int i = 0; i < CFArrayGetCount (windowList); i++)
					{
						CFDictionaryRef windowProperties = (CFDictionaryRef)CFArrayGetValueAtIndex (windowList, i);
						CGWindowLevel windowLevel;
						CFNumberGetValue ((CFNumberRef)CFDictionaryGetValue (windowProperties, (id)kCGWindowLayer), kCFNumberIntType, &windowLevel);
						// skip the dock, which lies on top of regular windows and unfortunately fills the whole screen
						if(windowLevel == dockWindowLevel)
							continue;
						// sometimes the cursor gets its own window
						if(windowLevel == cursorWindowLevel)
							continue;
						// accessibilty creates overlay windows
						if(windowLevel >= accessibiltyWindowLevel)
							continue;
						
						CGRectMakeWithDictionaryRepresentation ((CFDictionaryRef)CFDictionaryGetValue (windowProperties, (id)kCGWindowBounds), &windowBounds);
						if(CGRectContainsPoint (windowBounds, screenPoint))
						{
							CCL_PRINTF("Another window (%f %f %f %f level = %d) occludes (%f %f)\n", windowBounds.origin.x, windowBounds.origin.y, windowBounds.size.width, windowBounds.size.height, windowLevel, screenPoint.x, screenPoint.y)
							return nullptr;
						}
					}
				}
			}
			#endif
			return w;
		}
	EndForWindow
	
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaDesktopManager::countMonitors () const
{
#ifndef CCL_PLATFORM_IOS
	return (int)[[NSScreen screens] count];
#else
	return 1;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaDesktopManager::getMainMonitor () const
{
#ifndef CCL_PLATFORM_IOS
	NSUInteger index = [[NSScreen screens] indexOfObject:[NSScreen mainScreen]];
	return index == NSNotFound ? 0 : (int)index;	
#else
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CocoaDesktopManager::findMonitor (PointRef where, tbool defaultToPrimary) const
{
#ifndef CCL_PLATFORM_IOS
	NSScreen* iterScreen;
	int i = 0;
	Rect rect;
	for(iterScreen in [NSScreen screens])
	{
		NSRect nsRect = [iterScreen frame];
		nsRect.origin.y = MacOS::flipCoord (nsRect.origin.y) - nsRect.size.height;
		MacOS::fromNSRect (rect, nsRect);
		
		if(rect.pointInside (where))
			return i;
		i++;
	}
	return defaultToPrimary ? getMainMonitor () : -1;
#else
	return 0;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CocoaDesktopManager::getMonitorSize (Rect& rect, int index, tbool useWorkArea) const
{	
#ifndef CCL_PLATFORM_IOS
	if(index < [[NSScreen screens] count])
		if(NSScreen* screen = [[NSScreen screens] objectAtIndex:index])
		{
			NSRect nsRect = useWorkArea ? [screen visibleFrame] : [screen frame];
			nsRect.origin.y = MacOS::flipCoord (nsRect.origin.y) - nsRect.size.height;
			MacOS::fromNSRect (rect, nsRect);
			return true;
		}
			
	return false;

#else
	if(index < [[UIScreen screens] count])
		if(UIScreen* screen = [[UIScreen screens] objectAtIndex:index])
		{
			CGRect bounds = [screen bounds];
			if(useWorkArea)
			{
                if(screen == [UIScreen mainScreen])
                    if(UIWindow* window = [[[UIApplication sharedApplication] windows] firstObject])
                    {
                        UIEdgeInsets safeInsets = window.safeAreaInsets;
                        bounds.origin.x += safeInsets.left;
                        bounds.origin.y += safeInsets.top;
                        bounds.size.width -= (safeInsets.left + safeInsets.right);
                        bounds.size.height -= (safeInsets.top + safeInsets.bottom);
                    }
			}
			MacOS::fromCGRect (rect, bounds);
			return true;
		}
	
	return false;
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API CocoaDesktopManager::getMonitorScaleFactor (int index) const
{
#ifndef CCL_PLATFORM_IOS
	if(NSScreen* screen = [[NSScreen screens] objectAtIndex:index])
		return (float)[screen backingScaleFactor];
#else
	if(UIScreen* screen = [[UIScreen screens] objectAtIndex:index])
		return (float)[screen scale];
#endif
	return 1.f;
}
