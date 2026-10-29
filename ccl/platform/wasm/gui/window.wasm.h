//************************************************************************************************
//
// This file is part of Crystal Class Library (R)
// Copyright (c) 2026 CCL Software Licensing GmbH.
// All Rights Reserved.
//
// Licensed for use under either:
//  1. a Commercial License provided by CCL Software Licensing GmbH, or
//  2. GNU Affero General Public License v3.0 (AGPLv3).
// 
// You must choose and comply with one of the above licensing options.
// For more information, please visit ccl.dev.
//
// Filename    : ccl/platform/wasm/gui/window.wasm.h
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#ifndef _ccl_wasm_window_h
#define _ccl_wasm_window_h

#include "ccl/gui/windows/window.h"

#include "ccl/base/asyncoperation.h"

struct EmscriptenMouseEvent;
struct EmscriptenWheelEvent;
struct EmscriptenFocusEvent;
struct EmscriptenKeyboardEvent;
struct EmscriptenUiEvent;
struct EmscriptenTouchEvent;

namespace CCL {

class WasmDragSession;

//************************************************************************************************
// WasmWindow
//************************************************************************************************

class WasmWindow: public Window
{
public:
	DECLARE_CLASS_ABSTRACT (WasmWindow, Window)

	WasmWindow (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~WasmWindow ();

	enum StateFlags 
	{
		kConfigured = 1<<0, // window is visible
		kMoving = 1<<1, // window is being dragged
		kResizingWindow = 1<<2, // window is being resized
		kMaximized = 1<<3, // window follows page size
		kMouseCapture = 1<<4 // mouse is captured by this window
	};

	PROPERTY_FLAG (stateFlags, kConfigured, isConfigured)
	PROPERTY_FLAG (stateFlags, kMoving, isMoving)
	PROPERTY_FLAG (stateFlags, kResizingWindow, isResizingWindow)
	PROPERTY_FLAG (stateFlags, kMaximized, maximized)
	PROPERTY_FLAG (stateFlags, kMouseCapture, mouseCaptured)

	PROPERTY_MUTABLE_CSTRING (id, ID)

	bool getFrameId (MutableCString& frameId) const;
	bool getContentId (MutableCString& contentId) const;

	static WasmWindow* cast (Window* window) { return (WasmWindow*)window; } // hard cast, always has to work

	// Window
	EventResult handleEvent (SystemEvent& e) override;
	float CCL_API getContentScaleFactor () const override;
	tbool CCL_API setContentScaleFactor (float factor) override;
	void CCL_API moveWindow (PointRef pos) override;
	void CCL_API invalidate (RectRef rect) override;
	void CCL_API setUserSize (RectRef size) override;
	void CCL_API getUserSize (Rect& size) const override;
	void CCL_API getFrameSize (Rect& size) const override;
	void CCL_API maximize (tbool state) override;
	tbool CCL_API isMaximized () const override;
	void CCL_API center () override;
	void CCL_API redraw () override;
	tbool CCL_API isVisible () const override { return isConfigured (); }
	Point& CCL_API clientToScreen (Point& p) const override;
	Point& CCL_API screenToClient (Point& p) const override;
	tbool CCL_API close () override;
	void CCL_API setWindowTitle (StringRef title) override;
	bool isEnabled () const override { return true; }
	void resizeWindow (int edge) override;
	void moveWindow () override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseUp (const MouseEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onDragEnter (const DragEvent& event) override;
	bool captureMouse (bool state = true) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

protected:
	static constexpr int kDragTimeout = 200; // in ms
	static constexpr int kDragDistance = 2; // in coord units

	IWindow* parentWindow;
	int stateFlags;
	float savedDpiFactor;
	Point moveAnchor;
	Rect savedSize;

	struct DragItem
	{
		MutableCString mimeType;
		String data;
	};
	Vector<DragItem> dragItems;
	int dragEffect;
	WasmDragSession* dragSession;

	MouseEvent savedMouseDownEvent;
	bool mouseDownEventSent;

	static bool onMouseDownEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMouseUpEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onClickEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onCloseEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMaximizeEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onDoubleClickEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMouseMoveEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMoveWindowEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMouseEnterEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMouseLeaveEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data);
	static bool onMouseWheelEvent (int eventType, const EmscriptenWheelEvent* mouseEvent, void* data);
	static bool onFocusInEvent (int eventType, const EmscriptenFocusEvent* focusEvent, void* data);
	static bool onFocusOutEvent (int eventType, const EmscriptenFocusEvent* focusEvent, void* data);
	static bool onKeyDownEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data);
	static bool onKeyUpEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data);
	static bool onPageResized (int eventType, const EmscriptenUiEvent* uiEvent, void* data);
	static bool onTouchEvent (int eventType, const EmscriptenTouchEvent* touchEvent, void* data);

	void onToggleEvent (bool state);
	void onResizeEvent ();
	void onDpiChanged (float dpiFactor);
	void onDragEnterEffect (StringRef effect);
	void onDragEnterMimeType (int index, StringRef mimeType);
	void onDragEnterData (int index, StringRef data);
	int onDragEnterEvent (int x, int y);
	int onDragOverEvent (int x, int y);
	void onDragLeaveEvent ();
	void onDropData (int index, StringRef data);
	void onDropEvent (int x, int y);
	bool onDragStartEvent (int x, int y);

	void enableInput (bool state, bool capture = false);

	void onMoveWindow (int x, int y);
	void stopMoveWindow ();
	void setFrameSize (RectRef size);
	int getTitleBarHeight () const;

	// Window
	void setWindowSize (Rect& size) override;
	void showWindow (bool state = true, bool activate = true) override;
	void makeNativePopupWindow (IWindow* parent) override;
	void makeNativeChildWindow (void* nativeParent) override;
};

//************************************************************************************************
// WasmDialog
//************************************************************************************************

class WasmDialog: public WasmWindow
{
public:
	WasmDialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);

	PROPERTY_SHARED_POINTER (AsyncOperation, dialogOperation, DialogOperation)

	// WasmDialog
	tbool CCL_API close () override;
};

} // namespace CCL

#endif // _ccl_wasm_window_h
