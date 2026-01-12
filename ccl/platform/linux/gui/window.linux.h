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
// Filename    : ccl/platform/linux/gui/window.linux.h
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#ifndef _ccl_linux_window_h
#define _ccl_linux_window_h

#include "ccl/platform/linux/wayland/activationtoken.h"
#include "ccl/platform/linux/wayland/waylandchildwindow.h"

#include "ccl/platform/linux/linuxplatform.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/popup/inativepopup.h"

namespace CCL {

//************************************************************************************************
// LinuxWindow
//************************************************************************************************

class LinuxWindow: public Window,
				   public Linux::Surface,
				   public INativePopupSelectorWindow
{
public:
	LinuxWindow (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~LinuxWindow ();

	enum Flags 
	{
		kActive = 1<<0,
		kMaximized = 1<<1,
		kFullscreen = 1<<2,
		kConfigured = 1<<3,
		kPositionReconfigured = 1<<4,
		kInputEventsSuspended = 1<<5,
		kWantsFrameCallback = 1<<6
	};

	PROPERTY_FLAG (stateFlags, kActive, active)	
	PROPERTY_FLAG (stateFlags, kMaximized, maximized)
	PROPERTY_FLAG (stateFlags, kFullscreen, fullscreen)
	
	PROPERTY_FLAG (stateFlags, kConfigured, isConfigured)
	PROPERTY_FLAG (stateFlags, kPositionReconfigured, isPositionReconfigured)
	PROPERTY_FLAG (stateFlags, kInputEventsSuspended, inputEventsSuspended)
	PROPERTY_FLAG (stateFlags, kWantsFrameCallback, wantsFrameCallback)

	PROPERTY_OBJECT (Linux::WindowContext, windowContext, WindowContext)

	PROPERTY_POINTER (zxdg_toplevel_decoration_v1, decoration, Decoration)
	
	static LinuxWindow* cast (Window* window) { return reinterpret_cast<LinuxWindow*> (window); } // hard cast, always has to work
	static LinuxWindow* getMouseWindow ();
	
	void getSubSurfaces (Vector<LinuxWindow*>& subSurfaces) const;
	void getParentContextRecursive (Linux::WindowContext& context, bool includeSelf = false) const;
	Point getPositioningOffset () const;
	LinuxWindow* getParentLinuxWindow () const;
	
	void activate (CStringPtr token);
	void minimize ();
	void showMenu ();
	void discardSuspendedEvents ();

	const Linux::WindowContext* getNativeContext () const;
	
	// Window
	tbool CCL_API isSystemWindowValid () const override;
	float CCL_API getContentScaleFactor () const override;
	tbool CCL_API setContentScaleFactor (float factor) override;
	void CCL_API moveWindow (PointRef pos) override;
	void CCL_API setWindowTitle (StringRef title) override;
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
	void onActivate (bool state) override;
	tbool CCL_API isActive () const override;
	bool isEnabled () const override;
	tbool CCL_API close () override;
	void updateSize () override;
	void CCL_API getFrameSize (Rect& size) const override;
	void resizeWindow (int edge) override;
	Point& CCL_API clientToScreen (Point& p) const override;
	Point& CCL_API screenToClient (Point& pos) const override;
	bool setOpacity (float opacity) override;
	void CCL_API scrollClient (RectRef rect, PointRef delta) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	bool captureMouse (bool state = true) override;
	
	// Surface
	bool suppressInput () override;
	void setScaleFactor (int scaleFactor) override;
	void handleKeyboardEvent (const KeyEvent& event) override;
	void handleFocus (const FocusEvent& event) override;
	void handlePointerEvent (const Linux::InputHandler::PointerEvent& event) override;
	void handleTouchEvent (const Linux::InputHandler::TouchEvent& event) override;
	View* getView () override;
	void onCompositorConnected () override;
	void onCompositorDisconnected () override;
	
	// INativePopupSelectorWindow
	void setSizeInfo (const PopupSizeInfo& sizeInfo) override;
	
	CLASS_INTERFACES (Window)
	
protected:
	struct WindowListener: xdg_surface_listener,
						   xdg_toplevel_listener,
						   xdg_popup_listener,
						   zxdg_toplevel_decoration_v1_listener
						   #if WAYLAND_USE_XDG_ACTIVATION
						   ,xdg_activation_token_v1_listener
						   #endif
	{
		WindowListener (LinuxWindow& window);
		
		LinuxWindow& window;
		
		// surface
		static void onConfigureSurface (void* data, xdg_surface* surface, uint32_t serial);
		
		// top level
		static void onTopLevelConfigure (void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states);
		static void onClose (void* data, xdg_toplevel* toplevel);
		static void onConfigureBounds (void* data, xdg_toplevel* toplevel, int32_t width, int32_t height);
		static void onWindowManagerCapabilities (void *data, xdg_toplevel* xdg_toplevel, wl_array* capabilities);

		// popup
		static void onPopupConfigure (void* data, xdg_popup* popup, int32_t x, int32_t y, int32_t width, int32_t height);
		static void onPopupDone (void *data, xdg_popup *popup);
		static void onPopupRepositioned (void* data, xdg_popup* popup, uint32_t token);
		
		// decoration
		static void onConfigureDecoration (void* data, zxdg_toplevel_decoration_v1* decoration, uint32_t mode);
		
		#if WAYLAND_USE_XDG_ACTIVATION
		// activation
		static void onActivationDone (void* data, xdg_activation_token_v1* token, CStringPtr tokenString);
		#endif

	private:
		uint32_t nextDecorationMode;
	};
	WindowListener windowListener;
	
	AutoPtr<Linux::WaylandChildWindow> hostedChildWindow;
	Linux::SubSurface<> embeddedSubSurface;

	static LinuxWindow* mouseWindow;
	static LinuxWindow* activeChildWindow;
	static LinkedList<LinuxWindow*> popupStack;
	
	IWindow* parentWindow;
	int stateFlags;
	float savedDpiFactor;
	bool dismissPopup;
	
	LinkedList<MouseEvent*> suspendedEvents;
	LinkedList<KeyEvent*> suspendedKeyEvents;
	
	LinuxWindow* windowDecoration;
	
	PopupSizeInfo popupSizeInfo;
	
	#if WAYLAND_USE_XDG_ACTIVATION
	Linux::ActivationToken activationToken;
	#endif
	
	void onDpiChanged (float dpiFactor);
	virtual void applySize (RectRef clientSize);
	void applySizeInfo ();

	LinuxWindow* getTopLevelWindow () const;
	Point getFrameOffset () const;
	void enableClientSideDecoration (bool state = true);
	void requestActivationToken ();
	void onMaximize (bool state);
	void updateSizeLimits ();

	// Window
	void setWindowSize (Rect& size) override;
	void showWindow (bool state = true) override;
	void moveWindow () override;
	void fromNativeWindow (void* nativeWindow) override;
	void makeNativePopupWindow (IWindow* parent) override;
	void makeNativeChildWindow (void* nativeParent) override;
	void showPlatformInformation () override;
};

//************************************************************************************************
// LinuxDialog
//************************************************************************************************

class LinuxDialog: public LinuxWindow
{
public:
	LinuxDialog (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	
	// LinuxWindow
	tbool CCL_API close () override;
	
protected:
	tbool loopTerminated;

	#if WAYLAND_USE_XDG_DIALOG
	xdg_dialog_v1* xdgDialog;
	#endif
	
	// LinuxWindow
	void showWindow (bool state = true) override;
};

} // namespace CCL

#endif // _ccl_linux_window_h
