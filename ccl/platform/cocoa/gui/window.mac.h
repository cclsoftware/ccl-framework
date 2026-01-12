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
// Filename    : ccl/platform/cocoa/gui/window.mac.h
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#ifndef _ccl_mac_window_h
#define _ccl_mac_window_h

#include "ccl/gui/windows/window.h"

#include "ccl/public/gui/framework/itimer.h"

namespace CCL {
namespace MacOS {
class NativeView;
class QuartzBitmap;}

using namespace MacOS;

//************************************************************************************************
// OSXWindow
//************************************************************************************************

class OSXWindow: public Window
{
public:
	OSXWindow (const Rect& size = Rect (0, 0, 1, 1), StyleRef style = 0, StringRef title = nullptr);
	~OSXWindow ();

	static OSXWindow* cast (Window* window) { return (OSXWindow*)window; } // hard cast, always has to work

	void setFullscreenState (bool state);

	void embed (NativeView* subView);
	void onNativeViewRemoved ();
	void setNativeWindow (void *window) { handle = window; }
	NativeView* getNativeView () const { return nativeView; }
	void setNativeView (NativeView* nativeView);
	void forceActivate (Window* oldActiveWindow);
	QuartzBitmap* createScreenshotFromWindow () const;

	// Window
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	float CCL_API getContentScaleFactor () const override;
	void CCL_API moveWindow (PointRef pos) override;
	void CCL_API setWindowTitle (StringRef title) override;
	void CCL_API invalidate (RectRef rect) override;
	void CCL_API maximize (tbool state) override;
	tbool CCL_API isMaximized () const override;
	tbool CCL_API isMinimized () const override;
	void CCL_API setUserSize (RectRef size) override;
	void CCL_API getUserSize (Rect& size) const override;
	tbool CCL_API isVisible () const override;
	void CCL_API center () override;
	void CCL_API redraw () override;
	void CCL_API activate () override;
	tbool CCL_API isActive () const override;
	tbool CCL_API close () override;
	void updateSize () override;
	void setStyle (StyleRef style) override;
	void CCL_API getFrameSize (Rect& size) const override;
	void moveWindow () override;
	void resizeWindow (int edge = kEdgeBottomRight) override;
	Point& CCL_API clientToScreen (Point& p) const override;
	Point& CCL_API screenToClient (Point& pos) const override;
	bool setOpacity (float opacity) override;
	void CCL_API scrollClient (RectRef rect, PointRef delta) override;
	void beforeMouseDown (const MouseEvent& event) override;
	void updateBackgroundColor () override;
	bool isAttached () override;
	tbool CCL_API setFullscreen (tbool state) override;
	tbool CCL_API isFullscreen () const override;
	void onSize (const Point& delta) override;
	void* CCL_API getSystemWindow () const override;

protected:
	NativeView* nativeView;
	void* delegate;
	bool fullscreen;
	bool activating;
	PROPERTY_BOOL (suppressContextMenu, SuppressContextMenu)
	float savedOpacity;

	bool isChildWindow () const;
	void suppressTitleBar ();

	// Window
	void updateMenuBar () override;
	void setWindowSize (Rect& newSize) override;
	void showWindow (bool state) override;
	void makeNativePopupWindow (IWindow* parent) override;
	void makeNativeChildWindow (void* nativeParent) override;
};

//************************************************************************************************
// OSXDialog
//************************************************************************************************

class OSXDialog: public OSXWindow
{
public:
	OSXDialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	tbool close ();
	tbool loopTerminated;
	tbool repostMouseDown;
};

} // namespace CCL

#endif // _ccl_mac_window_h
