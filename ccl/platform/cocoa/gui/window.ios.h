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
// Filename    : ccl/platform/cocoa/gui/window.ios.h
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#ifndef _ccl_ios_window_h
#define _ccl_ios_window_h

#include "ccl/gui/windows/window.h"
#include "ccl/gui/popup/inativepopup.h"

namespace CCL {
namespace MacOS {
class NativeView;}

using namespace MacOS;

//************************************************************************************************
// IOSWindow
//************************************************************************************************

class IOSWindow: public Window
{
public:
	IOSWindow (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~IOSWindow ();

	static IOSWindow* cast (Window* window) { return (IOSWindow*)window; }

	NativeView* getNativeView () const { return nativeView; }
	void* getTopViewController () const;
	void updateSize ();
	
	// Window
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var);
	void invalidate (RectRef rect);
	void showWindow (bool state);
	tbool CCL_API close ();
	void getFrameSize (Rect& size) const;
	Point& CCL_API clientToScreen (CCL::Point& pos) const;
	Point& CCL_API screenToClient (CCL::Point& pos) const;
	float getContentScaleFactor () const;
	void CCL_API scrollClient (RectRef rect, PointRef delta);
	void CCL_API redraw ();
	void setWindowSize (Rect& newSize);
	void CCL_API center ();
	void updateBackgroundColor ();
	
protected:
	IWindow::StatusBarStyle statusBarStyle = IWindow::kLightContent;

	NativeView* nativeView;
	
	void applySafeAreaInsetsToChild (CCL::RectRef windowSize);

	// Window
	void makeNativePopupWindow (IWindow* parent);
};

//************************************************************************************************
// IOSDialog
//************************************************************************************************

class IOSDialog: public IOSWindow,
                 public INativePopupSelectorWindow
{
public:
	DECLARE_CLASS (IOSDialog, IOSWindow)

	tbool loopTerminated;
	tbool repostMouseDown;

	IOSDialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

    // IOSWindow
	tbool close () override;
	void updateSize () override;
	void setWindowSize (Rect& newSize) override;

	// INativePopupSelectorWindow
	void setSizeInfo (const PopupSizeInfo& sizeInfo) override { popupSizeInfo = sizeInfo; }

	CLASS_INTERFACE (INativePopupSelectorWindow, IOSWindow)

protected:
	PopupSizeInfo popupSizeInfo;
	bool updateSizeCalled;
};

} // namespace CCL

#endif // _ccl_ios_window_h
