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
// Filename    : ccl/platform/android/gui/window.android.h
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#ifndef _ccl_android_window_h
#define _ccl_android_window_h

#include "ccl/gui/windows/window.h"
#include "ccl/gui/popup/inativepopup.h"

#include "ccl/base/asyncoperation.h"

#include "ccl/platform/android/cclandroidjni.h"

namespace CCL {

namespace Android {
class FrameworkView; }

//************************************************************************************************
// AndroidWindow
//************************************************************************************************

class AndroidWindow: public Window
{
public:
	DECLARE_CLASS_ABSTRACT (AndroidWindow, Window)

	AndroidWindow (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~AndroidWindow ();

	static AndroidWindow* cast (Window* window) { return (AndroidWindow*)window; } // hard cast, always has to work

	PROPERTY_POINTER (Android::FrameworkView, frameworkView, FrameworkView)

	virtual bool isAppWindow () const;

	// Window
	using Window::invalidate;
	void CCL_API invalidate (RectRef rect) override;
	void CCL_API redraw () override;
	void draw (const UpdateRgn& updateRgn) override;
	void CCL_API scrollClient (RectRef rect, PointRef delta) override;
	bool isAttached () override;
	float getContentScaleFactor () const override;
	void makeNativePopupWindow (IWindow* parent) override;
	void showWindow (bool state) override;
	tbool CCL_API close () override;
	void setWindowSize (Rect& size) override;
	void CCL_API moveWindow (PointRef pos) override;
	void updateSize () override;
	Point& CCL_API clientToScreen (Point& pos) const override;
	Point& CCL_API screenToClient (Point& pos) const override;
	void CCL_API center () override;
	void updateBackgroundColor () override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;

private:
	bool ownsFrameworkView;
	bool isTranslucent;
	Rect initialSize;

	void adjustSizeToScreen (Android::FrameworkView& parentView);
};

//************************************************************************************************
// AndroidDialog
//************************************************************************************************

class AndroidDialog: public AndroidWindow,
					 public INativePopupSelectorWindow
{
public:
	AndroidDialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	PROPERTY_SHARED_AUTO (AsyncOperation, dialogOperation, DialogOperation)

	// Window
	void setWindowSize (Rect& size) override;
	void updateSize () override;
	tbool CCL_API close () override;
	void CCL_API scrollClient (RectRef rect, PointRef delta) override;

	// INativePopupSelectorWindow
	void setSizeInfo (const PopupSizeInfo& sizeInfo) override;

	CLASS_INTERFACE (INativePopupSelectorWindow, AndroidWindow)

protected:
	Android::JniObject dialog;
	PopupSizeInfo popupSizeInfo;
};

} // namespace CCL

#endif // _ccl_android_window_h
