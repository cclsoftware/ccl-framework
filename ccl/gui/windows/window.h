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
// Filename    : ccl/gui/windows/window.h
// Description : Window class
//
//************************************************************************************************

#ifndef _ccl_window_h
#define _ccl_window_h

#include "ccl/gui/windows/windowbase.h"

#include "ccl/gui/system/systemevent.h"

namespace CCL {

class MenuBar;
class MouseHandler;
class Settings;
class ThemeRenderer;
class TransparentWindow;
class NativeWindowRenderTarget;
class TouchInputState;

//************************************************************************************************
// Window Styles
//************************************************************************************************

namespace Styles
{
	extern StyleFlags defaultWindowStyle;
	extern StyleFlags sizableWindowStyle;
	extern StyleFlags panelWindowStyle;
	extern StyleFlags dialogWindowStyle;
}

//************************************************************************************************
// Window Layer
//************************************************************************************************

enum WindowLayer
{
	kWindowLayerBase,
	kWindowLayerIntermediate,
	kWindowLayerFloating,
	kDialogLayer,
	kPopupLayer = kDialogLayer,
	kNumWindowLayers
};

//************************************************************************************************
// WindowUpdateInfo
//************************************************************************************************

struct WindowUpdateInfo
{
	bool collectUpdates;
	IMutableRegion* region;
	Point offset;

	WindowUpdateInfo ()
	: collectUpdates (false),
	  region (nullptr)
	{}

	void addDirtyRect (RectRef rect)
	{
		if(region)
		{
			Rect r (rect);
			r.offset (offset);
			region->addRect (r);
		}
	}
};

//************************************************************************************************
// Window
//************************************************************************************************

class Window: public WindowBase,
			  public IWindow,
			  public SystemEventHandler
{
public:
	DECLARE_CLASS (Window, WindowBase)
	DECLARE_METHOD_NAMES (Window)

	Window (const Rect& size = Rect (), StyleRef style = 0, StringRef title = nullptr);
	~Window ();

	static Settings& getWindowSettings ();
	static void flushSettings ();
	static void cleanupSettings (); // used in terminator, treat private

	DECLARE_STYLEDEF (windowStyles)
	
	//////////////////////////////////////////////////////////////////////////////////////////////
	// IWindow
	//////////////////////////////////////////////////////////////////////////////////////////////

	tbool CCL_API isActive () const override;
	void CCL_API activate () override;
	void* CCL_API getSystemWindow () const override;
	tbool CCL_API isSystemWindowValid () const override;
	StringRef CCL_API getWindowTitle () const override;
	void CCL_API setWindowTitle (StringRef title) override;
	tbool CCL_API setCollectGraphicUpdates (tbool state) override;
	tbool CCL_API setCollectSizeUpdates (tbool state) override;
	float CCL_API getContentScaleFactor () const override;
	tbool CCL_API setContentScaleFactor (float factor) override;
	void CCL_API show () override;
	void CCL_API hide () override;
	void CCL_API maximize (tbool state) override;
	tbool CCL_API isMaximized () const override;
	tbool CCL_API isMinimized () const override;
	void CCL_API setUserSize (RectRef size) override;
	void CCL_API getUserSize (Rect& size) const override;
	tbool CCL_API isVisible () const override;
	void CCL_API redraw () override;
	void CCL_API center () override;
	tbool CCL_API close () override;
	void CCL_API popupContextMenu (PointRef where, tbool wasKeyPressed = false) override;
	void CCL_API moveWindow (PointRef pos) override;
	void CCL_API addHandler (IWindowEventHandler* handler) override;
	void CCL_API removeHandler (IWindowEventHandler* handler) override;
	IUnknown* CCL_API getController () const override;
	tbool CCL_API setController (IUnknown* u) override;
	IView* CCL_API getFocusIView () const override;
	void CCL_API getFrameSize (Rect& size) const override;
	tbool CCL_API setFullscreen (tbool state) override;
	tbool CCL_API isFullscreen () const override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// Internal methods
	//////////////////////////////////////////////////////////////////////////////////////////////

	enum WindowMode
	{
		kWindowModeRegular,		///< Window is used as regular top-level window (popups, dialogs, etc.)
		kWindowModeEmbedding,	///< Window is used to embedded framework view in foreign platform view
		kWindowModeHosting		///< Window is used to host a foreign platform view
	};
	
	PROPERTY_VARIABLE (WindowMode, windowMode, WindowMode)

	void setMenuBar (MenuBar* menuBar);
	MenuBar* getMenuBar () const;

	void deferActivate ();
	void deferClose ();
	void addToDesktop ();
	void tryMaximize (bool state);
	void inflate ();
	bool cancelDragSession ();
	void onViewRemoved (View* view);

	virtual void updateSize ();
	virtual void moveWindow ();
	enum Edge { kEdgeBottomRight, kEdgeLeft, kEdgeRight, kEdgeTop, kEdgeBottom };
	virtual void resizeWindow (int edge = kEdgeBottomRight);
	virtual void onResizing (bool begin);

	virtual bool onClose ();
	virtual void onDestroy ();
	virtual void onGestureProcessed (const GestureEvent& event, View* view);

	/** Capture the mouse. The window receives all mouse events until this method is called with \a state false or the window is closed. */
	virtual bool captureMouse (bool state = true);

	void setMouseHandler (MouseHandler* handler);
	MouseHandler* detachMouseHandler ();
	MouseHandler* getMouseHandler () const;
	bool setFocusView (View* view, bool directed = true);
	void killFocusView (bool permanent = false); ///< permanent: don't save for restoring later on window activation
	View* getFocusView () const;
	View* getSavedFocusView () const;
	bool canPopupContextMenu (PointRef where, bool wasKeyPressed = false) const;

	virtual bool setOpacity (float opacity);
	virtual float getOpacity () const;

	void addTransparentWindow (TransparentWindow* w);
	void removeTransparentWindow (TransparentWindow* w);
	TransparentWindow* getFirstTransparentWindow () const;
	CCL::Iterator* getTransparentWindows () const;

	int getZIndex () const; ///< returns the index of this window in the Z-order of all windows: topmost window has largest index
	WindowLayer getLayer () const;

	void redrawView (View* view);
	void redrawView (View* view, RectRef rect);
	void finishScroll (RectRef rect, PointRef delta);
	NativeWindowRenderTarget* getRenderTarget ();
	TouchInputState& getTouchInputState ();

	//////////////////////////////////////////////////////////////////////////////////////////////
	// View
	//////////////////////////////////////////////////////////////////////////////////////////////

	Window* getWindow () override;
	Window* getWindowForUpdate (WindowUpdateInfo& updateInfo) override;
	bool isAttached () override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void onActivate (bool state) override;
	void CCL_API setSize (RectRef size, tbool invalidate = true) override;
	void onSize (const Point& delta) override;
	void onChildSized (View* child, const Point& delta) override;
	void onChildLimitsChanged (View* child) override;
	void constrainSize (Rect& rect) const override;
	void setTitle (StringRef title) override;
	void onVisualStyleChanged () override;
	GraphicsDevice* getGraphicsDevice (Point& offset) override;
	using View::invalidate;
	void CCL_API invalidate (RectRef rect) override;
	void draw (const UpdateRgn& updateRgn) override;
	void CCL_API scrollClient (RectRef rect, PointRef delta) override;
	IGraphicsLayer* CCL_API getParentLayer (Point& offset) const override;
	void onDisplayPropertiesChanged (const DisplayChangedEvent& event) override;
	bool onMouseDown (const MouseEvent& event) override;
	bool onMouseMove (const MouseEvent& event) override;
	bool onMouseUp (const MouseEvent& event) override;
	bool onMouseWheel (const MouseWheelEvent& event) override;
	bool onFocus (const FocusEvent& event) override;
	bool onKeyDown (const KeyEvent& event) override;
	bool onKeyUp (const KeyEvent& event) override;
	bool onDragEnter (const DragEvent& event) override;
	bool onDragOver (const DragEvent& event) override;
	bool onDragLeave (const DragEvent& event) override;
	bool onDrop (const DragEvent& event) override;
	Point& CCL_API clientToWindow (Point& p) const override;
	Point& CCL_API clientToScreen (Point& p) const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	virtual AccessibilityProvider* getAccessibilityProvider () override;

	//////////////////////////////////////////////////////////////////////////////////////////////
	// SystemEventHandler
	//////////////////////////////////////////////////////////////////////////////////////////////

	EventResult handleEvent (SystemEvent& e) override;

	PROPERTY_BOOL (inDrawEvent, InDrawEvent)			///< true if window is about to draw itself
	PROPERTY_BOOL (inContextMenu, InContextMenu)		///< true if window is showing a context menu
	PROPERTY_BOOL (inMenuLoop, InMenuLoop)				///< true if window is inside a modal menu tracking loop
	PROPERTY_BOOL (inMoveLoop, InMoveLoop)				///< true if window is being moved
	PROPERTY_BOOL (inCloseEvent, InCloseEvent)			///< true if window is about to close
	PROPERTY_BOOL (inDestroyEvent, InDestroyEvent)		///< true if window is being closed
	PROPERTY_BOOL (collectUpdates, CollectUpdates)		///< true if window updates should be collected
	#if CCL_DEBUG_INTERNAL
	PROPERTY_BOOL (inRedrawView, InRedrawView)			///< true if inside an redrawView() call
	#endif

	bool shouldCollectUpdates () const { return collectUpdates; }
	bool shouldBeTranslucent () const;

	CLASS_INTERFACE (IWindow, WindowBase)


protected:
	friend class TooltipWindow;
	static Settings* windowSettings;

	LinkedList<IWindowEventHandler*> handlers;
	IUnknown* controller;
	MenuBar* menuBar;
	MouseHandler* mouseHandler;
	void* handle;
	float opacity;
	View* focusView;
	View* savedFocusView;
	ThemeRenderer* backgroundRenderer;
	ObjectList transparentWindows;
	WindowLayer layer;
	NativeWindowRenderTarget* renderTarget;
	TouchInputState* touchInputState;
	Point resizeStartSize;

	enum PrivateFlags
	{
		kInSetFocus = 1<<(kLastPrivateFlag + 1),
		kInUpdateSize = 1<<(kLastPrivateFlag + 2),
		kCollectResize =  1<<(kLastPrivateFlag + 3),
		kResizeDeferred = 1<<(kLastPrivateFlag + 4),
		kSizeRestored = 1<<(kLastPrivateFlag + 5),
		kResizeKeepRatio = 1<<(kLastPrivateFlag + 6)
	};

	PROPERTY_FLAG (privateFlags, kResizeDeferred, resizeDeferred)
	PROPERTY_FLAG (privateFlags, kCollectResize, collectResize)
	PROPERTY_FLAG (privateFlags, kSizeRestored, sizeRestored)
	PROPERTY_FLAG (privateFlags, kResizeKeepRatio, resizeKeepRatio)

	class ContextMenuBuilder;

	void destruct ();

	virtual StringRef getSettingsID (String&) const;
	virtual bool shouldStoreSize () const;
	virtual bool restoreSize ();
	virtual void storeSize ();
	virtual void drawBackground (const UpdateRgn& updateRgn);

	void initSize ();
	void saveFocusView (View* view);
	void moveWindowRectInsideScreen (Rect& windowRect);
	void limitSizeToScreen (Rect& windowRect);
	void tryMouseMove (const KeyEvent& event);
	void finishMouseHandler (MouseEvent& event, bool canceled);
	void signalWindowEvent (WindowEvent& windowEvent);
	bool canReceiveDrag () const;

	// platform-specific methods:
	virtual void updateMenuBar ();
	virtual void setWindowSize (Rect& size); // size may be adjusted
	virtual void showWindow (bool state = true);
	virtual void beforeMouseDown (const MouseEvent& event);
	virtual void updateBackgroundColor ();
	virtual void fromNativeWindow (void* nativeWindow);
	virtual void makeNativePopupWindow (IWindow* parent);
	virtual void makeNativeChildWindow (void* nativeParent);
	virtual void showPlatformInformation ();

	friend class DesktopManager;
	void setLayer (WindowLayer layer);

	// Object
	tbool CCL_API invokeMethod (Variant& returnValue, MessageRef msg) override;
};

//************************************************************************************************
// WindowGraphicsDevice
/** Graphics device for painting to a window (use Window::getGraphicsDevice). */
//************************************************************************************************

class WindowGraphicsDevice: public GraphicsDevice
{
public:
	WindowGraphicsDevice (Window& window, NativeGraphicsDevice* nativeDevice = nullptr);
	~WindowGraphicsDevice ();

protected:
	Window& window;
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// Window inline
//////////////////////////////////////////////////////////////////////////////////////////////////

inline MenuBar* Window::getMenuBar () const	 { return menuBar; }
inline MouseHandler* Window::getMouseHandler () const { return mouseHandler; }
inline View* Window::getFocusView () const { return focusView; }
inline float Window::getOpacity () const { return opacity; }
inline View* Window::getSavedFocusView () const { return savedFocusView; }
inline IView* CCL_API Window::getFocusIView () const { return focusView; }
inline void Window::onGestureProcessed (const GestureEvent& event, View* view) {}

//////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace CCL

#endif // _ccl_window_h
