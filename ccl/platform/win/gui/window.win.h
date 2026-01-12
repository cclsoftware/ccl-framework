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
// Filename    : ccl/platform/win/gui/window.win.h
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#ifndef _ccl_win32_window_h
#define _ccl_win32_window_h

#include "ccl/gui/windows/window.h"

namespace CCL {

//************************************************************************************************
// Win32Window
//************************************************************************************************

class Win32Window: public Window
{
public:
	Win32Window (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~Win32Window ();

	static Win32Window* cast (Window* window) { return (Win32Window*)window; } // hard cast, always has to work

	void sendNCActivate ();
	void suspendParent (bool state, void*& protectedData);
	void onDpiChanged (float dpiFactor, RectRef newPixelRect, bool suppressAdjustment = false);
	Point& screenPixelToClientCoord (Point& p) const;
	PointF& screenPixelToClientCoord (PointF& p) const;

	// Window
	float CCL_API getContentScaleFactor () const override;
	tbool CCL_API setContentScaleFactor (float factor) override;
	void CCL_API moveWindow (PointRef pos) override;
	void CCL_API setWindowTitle (StringRef title) override;
	void setStyle (StyleRef style) override;
	using Window::invalidate;
	void CCL_API invalidate (RectRef rect) override;
	void CCL_API maximize (tbool state) override;
	tbool CCL_API isMaximized () const override;
	tbool CCL_API isMinimized () const override;
	void CCL_API setUserSize (RectRef size) override;
	void CCL_API getUserSize (Rect& size) const override;
	tbool CCL_API setFullscreen (tbool state) override;
	tbool CCL_API isFullscreen () const override;
	tbool CCL_API isVisible () const override;
	void CCL_API center () override;
	void CCL_API redraw () override;
	void CCL_API activate () override;
	tbool CCL_API isActive () const override;
	bool isEnabled () const override;
	tbool CCL_API close () override;
	void updateSize () override;
	void CCL_API getFrameSize (Rect& size) const override;
	void moveWindow () override;
	void resizeWindow (int edge) override;
	Point& CCL_API clientToScreen (Point& p) const override;
	Point& CCL_API screenToClient (Point& pos) const override;
	bool setOpacity (float opacity) override;
	void CCL_API scrollClient (RectRef rect, PointRef delta) override;
	EventResult handleEvent (SystemEvent& e) override;
	bool captureMouse (bool state = true) override;

protected:
	static Win32Window* windowInResize;
	static Rect pendingWindowSize;

	float savedDpiFactor;
	bool inWheelEvent;

	struct FullscreenRestoreState
	{
		Rect size;
		StyleFlags style;
		bool maximized = false;
	};
	FullscreenRestoreState* fullscreenRestoreState;

	bool needsLayeredMode () const;
	bool needsLayeredRenderTarget () const;
	bool hasLayeredRenderTarget () const;
	void setLayeredRenderTarget (bool state);
	void setLayeredMode (bool state);

	bool hasVisibleMenuBar () const;

	// Window
	void updateMenuBar () override;
	void updateBackgroundColor () override;
	void setWindowSize (Rect& size) override;
	void showWindow (bool state = true) override;
	void fromNativeWindow (void* nativeWindow) override;
	void makeNativePopupWindow (IWindow* parent) override;
	void makeNativeChildWindow (void* nativeParent) override;
	void showPlatformInformation () override;
};

//************************************************************************************************
// Win32Dialog
//************************************************************************************************

class Win32Dialog: public Win32Window
{
public:
	Win32Dialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	static void beginModalMode (IWindow* dialog, bool state);

	// Win32Window
	EventResult handleEvent (SystemEvent& e) override;
};

} // namespace CCL

#endif // _ccl_win32_window_h
