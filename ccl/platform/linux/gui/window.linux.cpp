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
// Filename    : ccl/platform/win/gui/window.linux.cpp
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#define USE_SERVER_SIDE_DECORATION 1

#include "ccl/platform/linux/wayland/inputhandler.h"
#include "ccl/platform/linux/wayland/monitorhelper.h"
#include "ccl/platform/linux/wayland/subsurface.h"
#include "ccl/platform/linux/gui/window.linux.h"
#include "ccl/platform/linux/gui/nativewindowcontext.h"

#include "ccl/platform/shared/host/frameworkwindowdecoration.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/transparentwindow.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/popup/extendedmenu.h"
#include "ccl/gui/help/helpmanager.h"
#include "ccl/gui/system/dragndrop.h"
#include "ccl/gui/system/mousecursor.h"
#include "ccl/gui/touch/touchinput.h"
#include "ccl/gui/touch/gesturemanager.h"
#include "ccl/gui/graphics/nativegraphics.h"

#include "ccl/base/message.h"
#include "ccl/base/signalsource.h"

#include "ccl/public/systemservices.h"
#include "ccl/public/system/isignalhandler.h"
#include "ccl/public/system/ilocalemanager.h"
#include "ccl/public/math/mathprimitives.h"
#include "ccl/public/gui/framework/controlsignals.h"
#include "ccl/public/cclversion.h"

namespace CCL {
namespace Linux {

//************************************************************************************************
// SubSurfaceWindow
//************************************************************************************************

class SubSurfaceWindow: public SubSurface<LinuxWindow>
{
public:
	SubSurfaceWindow (LinuxWindow& parent);
	
	// LinuxWindow
	void showWindow (bool state = true) override;
	void CCL_API moveWindow (PointRef pos) override;
	tbool CCL_API isVisible () const override;
	tbool CCL_API isActive () const override;
	Point& CCL_API clientToScreen (Point& pos) const override;
	Point& CCL_API screenToClient (Point& pos) const override;
	void CCL_API invalidate (RectRef rect) override;

	// SubSurface
	void handleKeyboardEvent (const KeyEvent& event) override;
	void handleFocus (const FocusEvent& event) override;
	void handlePointerEvent (const InputHandler::PointerEvent& event) override;
};

//************************************************************************************************
// LinuxWindowDecorationController
//************************************************************************************************

class LinuxWindowDecorationController: public WindowDecorationController
{
protected:
	// WindowDecorationController
	void onMinimize ();
	void onShowMenu ();
};

//************************************************************************************************
// LinuxWindowDecoration
//************************************************************************************************

class LinuxWindowDecoration: public SubSurfaceWindow
{
public:
	LinuxWindowDecoration (LinuxWindow& parent);
	~LinuxWindowDecoration ();
	
	static const int kResizeBorderRadius = 3;
	static const int kResizeCornerRadius = 16;
	
	Coord getBorderWidth () const;
	Coord getTitleBarHeight () const;

	void update ();
	
	// SubSurfaceWindow
	void showWindow (bool state = true) override;
	void applySize (RectRef size) override;
	tbool CCL_API close () override;
	void handlePointerEvent (const InputHandler::PointerEvent& event) override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	
protected:
	LinuxWindowDecorationController controller;
	ThemeCursorID currentCursorId;
	
	void handleResize (const InputHandler::PointerEvent& event);
	void updateTitleBarHeight ();
};

//************************************************************************************************
// LinuxPopupMenu
//************************************************************************************************

class LinuxPopupMenu: public ExtendedPopupMenu
{
public:
	DECLARE_CLASS (LinuxPopupMenu, ExtendedPopupMenu)
};

} // namespace Linux
} // namespace CCL

using namespace CCL;
using namespace Linux;

//////////////////////////////////////////////////////////////////////////////////////////////////

CCL_KERNEL_INIT (LinuxPopupMenu)
{
	// Always use custom menu bar on Linux
	GUI.setCustomMenuBarSupported (true);
	Configuration::Registry::instance ().setValue ("GUI.ApplicationWindow", "CustomMenuBar", true);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Use ExtendedMenuBar as platform menu implementation
//////////////////////////////////////////////////////////////////////////////////////////////////

DEFINE_CLASS_UID (ExtendedMenuBar, 0x32ac7729, 0x5ee3, 0x4273, 0xaf, 0x9d, 0xaf, 0x50, 0x1e, 0x7c, 0xe5, 0xb0) // ClassID::MenuBar
DEFINE_CLASS_UID (ExtendedVariantMenuBar, 0xd0d769c9, 0xe469, 0x445a, 0xb1, 0x9, 0x66, 0x7f, 0x55, 0xe1, 0xa0, 0xf5) // ClassID::VariantMenuBar

DEFINE_CLASS_PERSISTENT (LinuxPopupMenu, ExtendedPopupMenu, "Menu")

//************************************************************************************************
// LinuxWindow
//************************************************************************************************

LinuxWindow* LinuxWindow::mouseWindow = nullptr;
LinuxWindow* LinuxWindow::activeChildWindow = nullptr;
LinkedList<LinuxWindow*> LinuxWindow::popupStack;

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxWindow::LinuxWindow (const Rect& size, StyleRef style, StringRef title)
: Window (size, style, title),
  stateFlags (0),
  windowContext {nullptr},
  embeddedSubSurface (*this),
  decoration (nullptr),
  parentWindow (nullptr),
  windowListener (*this),
  savedDpiFactor (1.f),
  dismissPopup (false),
  windowDecoration (nullptr),
  popupSizeInfo (Point ())
{
	wantsFrameCallback (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxWindow::~LinuxWindow ()
{
	discardSuspendedEvents ();
	destruct ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API LinuxWindow::queryInterface (UIDRef iid, void** ptr)
{
	if(hostedChildWindow != nullptr && iid == ccl_iid<IWaylandChildWindow> ())
		return hostedChildWindow->queryInterface (iid, ptr);
	QUERY_INTERFACE (INativePopupSelectorWindow)
	return Window::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxWindow* LinuxWindow::getMouseWindow ()
{
	return mouseWindow;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::getSubSurfaces (Vector<LinuxWindow*>& subSurfaces) const
{
	if(windowDecoration)
		subSurfaces.add (windowDecoration);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(windowDecoration)
	{
		windowDecoration->onColorSchemeChanged (event);
		windowDecoration->redraw ();
	}
	
	Window::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::fromNativeWindow (void* nativeHandle)
{
	NativeWindowContext* nativeContext = static_cast<NativeWindowContext*> (nativeHandle);
	if(nativeContext)
	{
		windowContext.topLevelWindow = nativeContext->topLevelWindow;
		windowContext.popupWindow = nativeContext->popupWindow;
		parentWindow = nativeContext->parent;
		
		if(nativeContext->topLevelWindow)
		{
			WindowContext parentContext;
			getParentContextRecursive (parentContext);
			if(nativeContext->parent && parentContext.topLevelWindow)
				xdg_toplevel_set_parent (nativeContext->topLevelWindow, parentContext.topLevelWindow);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::makeNativePopupWindow (IWindow* parent)
{
	handle = &windowContext;
	parentWindow = parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::makeNativeChildWindow (void* nativeParent)
{
	ASSERT (nativeParent != nullptr)
	if(windowMode == kWindowModeHosting)
	{
		handle = nativeParent;
		hostedChildWindow = NEW WaylandChildWindow (*this);
	}
	else if(nativeParent != nullptr)
	{
		handle = &windowContext;
		windowContext = *static_cast<WindowContext*> (nativeParent);
	}
	moveWindow (size.getLeftTop ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

const WindowContext* LinuxWindow::getNativeContext () const
{
	return handle ? static_cast<WindowContext*> (getSystemWindow ()) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::showPlatformInformation ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::isSystemWindowValid () const
{
	return handle != nullptr && windowContext.waylandSurface != nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API LinuxWindow::getContentScaleFactor () const
{
	return savedDpiFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::setContentScaleFactor (float factor)
{
	onDpiChanged (factor);
	
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::onDpiChanged (float dpiFactor)
{
	if(windowContext.waylandSurface)
	{
		if(dpiFactor != savedDpiFactor)
		{
			onDisplayPropertiesChanged (DisplayChangedEvent (dpiFactor, DisplayChangedEvent::kResolutionChanged));
			
			Rect r;
			getClientRect (r);
			invalidate (r);
			
			savedDpiFactor = dpiFactor;
			if(isConfigured ())
				if(NativeWindowRenderTarget* target = getRenderTarget ())
					target->onSize ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::discardSuspendedEvents ()
{
	while(MouseEvent* event = suspendedEvents.removeFirst ())
		delete event;
	while(KeyEvent* event = suspendedKeyEvents.removeFirst ())
		delete event;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::updateSizeLimits ()
{
	if(windowContext.topLevelWindow)
	{
		Rect frameSize;
		getFrameSize (frameSize);
		
		if(getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable))
		{
			const SizeLimit& sizeLimits = getSizeLimits ();

			Rect size = getSize ();
			Point additionalSize = frameSize.getSize () - size.getSize ();

			if(sizeLimits.minWidth >= 0 && sizeLimits.minHeight >= 0)
				xdg_toplevel_set_min_size (windowContext.topLevelWindow, sizeLimits.minWidth + additionalSize.x, sizeLimits.minHeight + additionalSize.y);
			if(sizeLimits.maxWidth >= 0 && sizeLimits.maxHeight >= 0)
				xdg_toplevel_set_max_size (windowContext.topLevelWindow, sizeLimits.maxWidth + additionalSize.x, sizeLimits.maxHeight + additionalSize.y);
		}
		else
		{
			xdg_toplevel_set_min_size (windowContext.topLevelWindow, frameSize.getWidth (), frameSize.getHeight ());
			xdg_toplevel_set_max_size (windowContext.topLevelWindow, frameSize.getWidth (), frameSize.getHeight ());
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::enableClientSideDecoration (bool state)
{
	if(state == false)
	{
		if(windowDecoration != nullptr)
			windowDecoration->close ();
		windowDecoration = nullptr;
	}
	else if(windowDecoration == nullptr)
	{
		LinuxWindowDecoration* decoration = NEW LinuxWindowDecoration (*this);
		windowDecoration = decoration;
		windowDecoration->show ();
		decoration->placeBelow (*this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::setSizeInfo (const PopupSizeInfo& sizeInfo)
{
	popupSizeInfo = sizeInfo;
	applySizeInfo ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxWindow::captureMouse (bool state)
{
	Point initialPosition;
	GUI.getMousePosition (initialPosition);
	screenToClient (initialPosition);

	return InputHandler::instance ().grabPointer (*this, state, initialPosition);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::applySizeInfo ()
{
	if(windowContext.positioner)
	{
		Point positioningOffset = getPositioningOffset ();
		Point position (popupSizeInfo.where);
		if(popupSizeInfo.parent)
		{
			popupSizeInfo.parent->clientToScreen (position);

			IWindow* parentWindow = popupSizeInfo.parent->getIWindow ();
			if(parentWindow)
			{
				Rect parentWindowSize;
				parentWindow->getFrameSize (parentWindowSize);

				if(popupSizeInfo.flags & PopupSizeInfo::kHFillWindow)
				{
					ccl_lower_limit (position.x, parentWindowSize.left);
					ccl_upper_limit (position.x, parentWindowSize.right - size.getWidth ());
				}
				if(popupSizeInfo.flags & PopupSizeInfo::kVFillWindow)
				{
					ccl_lower_limit (position.y, parentWindowSize.top);
					ccl_upper_limit (position.y, parentWindowSize.bottom - size.getHeight ());
				}
			}
		}
		position.offset (positioningOffset);
		
		Rect anchorRect (popupSizeInfo.anchorRect);
		if(!anchorRect.isEmpty ())
			anchorRect.offset (positioningOffset);
		
		Point offset;
			
		if(popupSizeInfo.parent)
		{
			bool left = (popupSizeInfo.flags & PopupSizeInfo::kLeft) != 0;
			bool right = (popupSizeInfo.flags & PopupSizeInfo::kRight) != 0;
			bool hCenter = (popupSizeInfo.flags & PopupSizeInfo::kHCenter) != 0;
			bool top = (popupSizeInfo.flags & PopupSizeInfo::kTop) != 0;
			bool bottom = (popupSizeInfo.flags & PopupSizeInfo::kBottom) != 0;
			bool vCenter = (popupSizeInfo.flags & PopupSizeInfo::kVCenter) != 0;
			
			if(left && !right)
				offset.x = 0;
			else if(right && !left)
				offset.x = popupSizeInfo.parent->getSize ().getWidth () - size.getWidth ();
			else if(hCenter)
				offset.x = popupSizeInfo.parent->getSize ().getWidth () / 2 - size.getWidth () / 2;
			
			if(top && !bottom)
				offset.y = -size.getHeight ();
			else if(bottom && !top)
				offset.y = popupSizeInfo.parent->getSize ().getHeight ();
			else if(vCenter)
				offset.y = popupSizeInfo.parent->getSize ().getHeight () / 2 - size.getHeight () / 2;
		}
		
		xdg_positioner_set_size (windowContext.positioner, size.getWidth (), size.getHeight ());
		xdg_positioner_set_anchor (windowContext.positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
		
		if(anchorRect.isEmpty ())
		{
			xdg_positioner_set_anchor_rect (windowContext.positioner, 0, 0, 1, 1);
			xdg_positioner_set_offset (windowContext.positioner, position.x + offset.x, position.y + offset.y);
		}
		else
		{
			xdg_positioner_set_anchor_rect (windowContext.positioner, 0, 0, anchorRect.getWidth (), anchorRect.getHeight ());
			xdg_positioner_set_offset (windowContext.positioner, anchorRect.left + offset.x, anchorRect.top + offset.y);
		}
		xdg_positioner_set_gravity (windowContext.positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
		
		uint32_t constraintAdjustment = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_NONE;
		if((popupSizeInfo.flags & PopupSizeInfo::kForceFixedPosition) == 0)
		{
			constraintAdjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X;
			constraintAdjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y;
		}
		if((popupSizeInfo.flags & PopupSizeInfo::kCanFlipParentEdge) == 0)
		{
			constraintAdjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X;
			constraintAdjustment |= XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_Y;
		}
		xdg_positioner_set_constraint_adjustment (windowContext.positioner, constraintAdjustment);
		
		if(windowContext.popupWindow && xdg_popup_get_version (windowContext.popupWindow) >= XDG_POPUP_REPOSITION_SINCE_VERSION)
			xdg_popup_reposition (windowContext.popupWindow, windowContext.positioner, 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::moveWindow (PointRef pos)
{
	if(hostedChildWindow != nullptr)
	{
		hostedChildWindow->setPosition (pos);
		hostedChildWindow->commit ();
		size.moveTo (pos);
		return;
	}

	if(size.getLeftTop () == pos)
		return;

	if(windowContext.popupWindow && windowContext.positioner && xdg_popup_get_version (windowContext.popupWindow) >= XDG_POPUP_REPOSITION_SINCE_VERSION)
	{
		Point position = getPositioningOffset ();
		position.offset (pos);
		xdg_positioner_set_size (windowContext.positioner, size.getWidth (), size.getHeight ());
		xdg_positioner_set_anchor_rect (windowContext.positioner, 0, 0, 1, 1);
		xdg_positioner_set_offset (windowContext.positioner, position.x, position.y);
		xdg_popup_reposition (windowContext.popupWindow, windowContext.positioner, 0);
		size.moveTo (pos);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::moveWindow ()
{
	WaylandClient& client = WaylandClient::instance ();
	if(windowContext.topLevelWindow)
	{
		if(wl_seat* seat = client.getSeat ())
			xdg_toplevel_move (windowContext.topLevelWindow, seat, InputHandler::instance ().getSerial ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::setWindowTitle (StringRef title)
{
	if(windowContext.topLevelWindow)
		xdg_toplevel_set_title (windowContext.topLevelWindow, MutableCString (title, Text::kUTF8).str ());
	SuperClass::setTitle (title);
	if(windowDecoration)
		static_cast<LinuxWindowDecoration*> (windowDecoration)->update ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::invalidate (RectRef rect)
{
	if(!isConfigured () || isInDestroyEvent ())
		return;
	
	if(NativeWindowRenderTarget* target = getRenderTarget ())
		if(IMutableRegion* region = target->getInvalidateRegion ())
			region->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::onCompositorDisconnected ()
{
	showWindow (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::onCompositorConnected ()
{
	showWindow (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::showWindow (bool state)
{
	if(hostedChildWindow != nullptr)
	{
		hostedChildWindow->show (state);
		enableInput (state);

		if(!state && activeChildWindow == this)
			activeChildWindow = nullptr;

		return;
	}
		
	bool isEmbeddingChildWindow = windowMode == kWindowModeEmbedding;

	if(state && getWaylandSurface () == nullptr)
	{
		WaylandClient& client = WaylandClient::instance ();
	
		wl_compositor* compositor = client.getCompositor ();
		wl_display* display = client.getDisplay ();
		xdg_wm_base* windowManager = client.getWindowManager ();
		if(compositor == nullptr || windowManager == nullptr || display == nullptr)
			return;	
		
		bool wantsDecoration = (style.isCustomStyle (Styles::kWindowBehaviorSizable)
									|| (style.isCustomStyle (Styles::kWindowAppearanceTitleBar) && !style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))) 
								&& !style.isCustomStyle (Styles::kWindowBehaviorPopupSelector);
		bool wantsGrab = style.isCustomStyle (Styles::kWindowBehaviorPopupSelector);
		
		bool isTopLevelWindow = !isEmbeddingChildWindow && parentWindow == nullptr;
		isTopLevelWindow |= !style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) && !style.isCustomStyle (Styles::kWindowBehaviorTooltip) && !style.isCustomStyle (Styles::kWindowBehaviorFloating);
		bool isPopupWindow = !isTopLevelWindow;

		if(isEmbeddingChildWindow)
		{
			isTopLevelWindow = false;
			isPopupWindow = false;
		}
		else
		{
			if(getWaylandSurface () == nullptr)
				createSurface ();

			windowContext.waylandSurface = getWaylandSurface ();
			if(windowContext.waylandSurface == nullptr)
			{
				CCL_WARN ("Failed to create a Wayland surface.\n", 0)
				return;
			}
		
			if(parentWindow)
				setScaleFactor (parentWindow->getContentScaleFactor ());
			
			windowContext.xdgSurface = xdg_wm_base_get_xdg_surface (windowManager, windowContext.waylandSurface);
			if(windowContext.xdgSurface == nullptr)
			{
				CCL_WARN ("Failed to create an XDG surface.\n", 0)
				return;
			}
			
			xdg_surface_add_listener (windowContext.xdgSurface, &windowListener, &windowListener);	
		}

		WindowContext parentContext;
		getParentContextRecursive (parentContext);
		
		if(isTopLevelWindow)
		{
			windowContext.topLevelWindow = xdg_surface_get_toplevel (windowContext.xdgSurface);
			if(windowContext.topLevelWindow == nullptr)
			{
				CCL_WARN ("Failed to create a top-level window.\n", 0)
				return;
			}
			xdg_toplevel_add_listener (windowContext.topLevelWindow, &windowListener, &windowListener);	
			
			updateSizeLimits ();
			
			#if USE_SERVER_SIDE_DECORATION
			if(wantsDecoration && !style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
			{
				zxdg_decoration_manager_v1* decorationManager = client.getDecorationManager ();
				if(decorationManager != nullptr)
				{
					decoration = zxdg_decoration_manager_v1_get_toplevel_decoration (decorationManager, windowContext.topLevelWindow);
					if(decoration)
					{
						zxdg_toplevel_decoration_v1_add_listener (decoration, &windowListener, &windowListener);
						zxdg_toplevel_decoration_v1_set_mode (decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
					}
				}
			}
			#endif
			
			xdg_toplevel_set_app_id (windowContext.topLevelWindow, client.getApplicationId ());
			
			if(parentWindow)
			{
				if(parentContext.topLevelWindow)
					xdg_toplevel_set_parent (windowContext.topLevelWindow, parentContext.topLevelWindow);
			}
			
			setWindowTitle (title);
		}
		
		if(isPopupWindow)
		{
			windowContext.positioner = xdg_wm_base_create_positioner (windowManager);
			
			xdg_positioner_set_size (windowContext.positioner, size.getWidth (), size.getHeight ());
			
			Point position = getPositioningOffset ();
			position.offset (size.getLeftTop ());
			xdg_positioner_set_anchor_rect (windowContext.positioner, 0, 0, 1, 1);
			xdg_positioner_set_offset (windowContext.positioner, position.x, position.y);
			xdg_positioner_set_anchor (windowContext.positioner, XDG_POSITIONER_ANCHOR_TOP_LEFT);
			xdg_positioner_set_gravity (windowContext.positioner, XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
			xdg_positioner_set_constraint_adjustment (windowContext.positioner, XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_NONE);
			
			if(popupSizeInfo.parent != nullptr)
				applySizeInfo ();
			
			if(parentContext.xdgSurface)
				windowContext.popupWindow = xdg_surface_get_popup (windowContext.xdgSurface, parentContext.xdgSurface, windowContext.positioner);

			while(!popupStack.isEmpty () && popupStack.getLast () != getParentLinuxWindow ())
				popupStack.getLast ()->close ();
			popupStack.append (this);
						
			xdg_popup_add_listener (windowContext.popupWindow, &windowListener, &windowListener);
			
			if(wantsGrab)
			{
				uint32_t serial = InputHandler::instance ().getSerial ();
				InputHandler::instance ().setSerial (0);
				
				if(serial > 0)
					xdg_popup_grab (windowContext.popupWindow, client.getSeat (), serial);
			}
		}

		if(isEmbeddingChildWindow)
		{
			setWaylandSurface (windowContext.waylandSurface);
			embeddedSubSurface.createSurface ();
			embeddedSubSurface.setSynchronous (false);
			embeddedSubSurface.enableInput ();
			setWaylandSurface (embeddedSubSurface.getWaylandSurface ());
			isConfigured (true);
			applySize (size);
		}
		else
			enableInput ();

		commit ();
				
		if(!isEmbeddingChildWindow)
		{
			bool needsClientSideDecoration = wantsDecoration && (isPopupWindow || decoration == nullptr);
			enableClientSideDecoration (needsClientSideDecoration);
		
			updateMenuBar ();
		}

		if(graphicsLayer)
			graphicsLayer->flush ();
	}
	else if(!state && getWaylandSurface () != nullptr)
	{
		if(popupStack.contains (this))
		{
			while(popupStack.getLast () != this)
				popupStack.getLast ()->hide ();
			ASSERT (popupStack.getLast () == this)
			popupStack.removeLast ();
		}
		
		if(mouseWindow == this)
			mouseWindow = nullptr;
		
		enableClientSideDecoration (false);
		
		enableInput (false);
		
		#if WAYLAND_USE_XDG_ACTIVATION
		if(WaylandClient::instance ().isInitialized ())
			activationToken.reset ();
		#endif

		if(decoration && WaylandClient::instance ().isInitialized ())
			zxdg_toplevel_decoration_v1_destroy (decoration);
		decoration = nullptr;

		if(!isEmbeddingChildWindow)
		{
			if(windowContext.positioner && WaylandClient::instance ().isInitialized ())
				xdg_positioner_destroy (windowContext.positioner);
			windowContext.positioner = nullptr;

			if(windowContext.topLevelWindow && WaylandClient::instance ().isInitialized ())
				xdg_toplevel_destroy (windowContext.topLevelWindow);
			windowContext.topLevelWindow = nullptr;
			
			if(windowContext.popupWindow && WaylandClient::instance ().isInitialized ())
				xdg_popup_destroy (windowContext.popupWindow);
			windowContext.popupWindow = nullptr;
			
			if(windowContext.xdgSurface && WaylandClient::instance ().isInitialized ())
				xdg_surface_destroy (windowContext.xdgSurface);
			windowContext.xdgSurface = nullptr;

			windowContext.waylandSurface = nullptr;
		}
		
		isConfigured (false);
		safe_release (renderTarget);

		if(isEmbeddingChildWindow)
		{
			embeddedSubSurface.enableInput (false);
			embeddedSubSurface.destroySurface ();
			setWaylandSurface (nullptr);
		}
		else
			destroySurface ();
	}

	deferSignal (NEW Message (kSystemWindowChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::showMenu ()
{
	if(windowContext.topLevelWindow != nullptr)
	{
		WaylandClient& client = WaylandClient::instance ();
		Point mousePosition;
		GUI.getMousePosition (mousePosition);
		screenToClient (mousePosition);
		mousePosition.offset (getFrameOffset ());
		xdg_toplevel_show_window_menu (windowContext.topLevelWindow, client.getSeat (), client.getSerial (), mousePosition.x, mousePosition.y);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::isMinimized () const
{
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::applySize (const Rect& _size)
{
	Rect size (_size);
	View::setSize (getSizeLimits ().makeValid (size));

	Rect totalFrameSize = getSize ();
	if(windowContext.topLevelWindow != nullptr)
		totalFrameSize = totalFrameSize.getSize ();
	
	LinuxWindowDecoration* decoration = static_cast<LinuxWindowDecoration*> (windowDecoration);
	if(decoration)
	{
		totalFrameSize.expand (decoration->getBorderWidth ());
		totalFrameSize.top += decoration->getBorderWidth () - decoration->getTitleBarHeight ();
		Rect decorationSize (totalFrameSize);
		if(windowContext.topLevelWindow == nullptr)
			decorationSize.offset (-size.left, -size.top);
		windowDecoration->setSize (decorationSize);
	}
		
	Rect r;
	getClientRect (r);
	invalidate (r);
	
	if(isConfigured ())
		if(NativeWindowRenderTarget* target = getRenderTarget ())
			target->onSize ();
	
	if(windowContext.topLevelWindow != nullptr)
		xdg_surface_set_window_geometry (windowContext.xdgSurface, totalFrameSize.left, totalFrameSize.top, totalFrameSize.getWidth (), totalFrameSize.getHeight ());
	
	if(getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable) && windowContext.waylandSurface != nullptr)
	{
		wl_region* region = wl_compositor_create_region (WaylandClient::instance ().getCompositor ());
		if(region)
		{
			Rect inputSize = getSize ().getSize ();
			if(decoration != nullptr)
			{
				int effectiveBorderRadius = ccl_max (decoration->getBorderWidth (), LinuxWindowDecoration::kResizeBorderRadius);
				int effectiveTopBorderRadius = ccl_max (LinuxWindowDecoration::kResizeBorderRadius - decoration->getTitleBarHeight (), 0);
				inputSize.contract (effectiveBorderRadius);
				inputSize.top += effectiveTopBorderRadius - effectiveBorderRadius;
			}
			wl_region_add (region, inputSize.left, inputSize.top, inputSize.getWidth (), inputSize.getHeight ());
			wl_surface_set_input_region (windowContext.waylandSurface, region);
			wl_region_destroy (region);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::getFrameSize (Rect& size) const
{
	Rect totalFrameSize = getSize ();
	if(windowContext.topLevelWindow != nullptr)
		totalFrameSize = totalFrameSize.getSize ();
	
	if(windowDecoration)
	{
		LinuxWindowDecoration* decoration = static_cast<LinuxWindowDecoration*> (windowDecoration);
		totalFrameSize.expand (decoration->getBorderWidth ());
		totalFrameSize.top += decoration->getBorderWidth () - decoration->getTitleBarHeight ();
	}
	
	size = totalFrameSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::setWindowSize (Rect& newSize)
{
	moveWindow ({ newSize.left, newSize.top });
	setUserSize (newSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::setUserSize (RectRef _size)
{
	Rect newSize (_size);
	constrainSize (newSize);
	
	if(newSize.getWidth () == 0)
		newSize.setWidth (1);
		
	if(newSize.getHeight () == 0)
		newSize.setHeight (1);
	
	if(hostedChildWindow)
		hostedChildWindow->setUserSize (newSize);

	applySize (newSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::getUserSize (Rect& userSize) const
{
	userSize = getSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::setFullscreen (tbool state)
{
	if(windowContext.topLevelWindow != nullptr)
	{
		if(state)
			xdg_toplevel_set_fullscreen (windowContext.topLevelWindow, output);
		else
			xdg_toplevel_unset_fullscreen (windowContext.topLevelWindow);
		return fullscreen ();
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::isFullscreen () const
{
	return fullscreen ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::isVisible () const
{
	return isConfigured ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::center ()
{
	if(parentWindow)
	{
		Rect size;
		parentWindow->getFrameSize (size);
		Point position = size.getLeftTop ();
		position.offset ((size.getWidth () - getWidth ()) / 2, (size.getHeight () - getHeight ()) / 2);
		moveWindow (position);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::redraw ()
{
	Rect r;
	getClientRect (r);
	invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::activate (CStringPtr token)
{
	#if WAYLAND_USE_XDG_ACTIVATION
	wl_surface* surface = getWaylandSurface ();
	xdg_activation_v1* activation = WaylandClient::instance ().getActivation ();
	if(activation != nullptr && token != nullptr && surface != nullptr)
		xdg_activation_v1_activate (activation, token, surface);

	activationToken.reset ();
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::minimize ()
{
	if(windowContext.topLevelWindow != nullptr)
		xdg_toplevel_set_minimized (windowContext.topLevelWindow);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::maximize (tbool state)
{
	if(windowContext.topLevelWindow != nullptr)
	{
		if(state)
			xdg_toplevel_set_maximized (windowContext.topLevelWindow);
		else
			xdg_toplevel_unset_maximized (windowContext.topLevelWindow);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::isMaximized () const
{
	return maximized ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::onMaximize (bool state)
{
	maximized (state);
	WindowEvent maximizeEvent (*this, state ? WindowEvent::kMaximize : WindowEvent::kUnmaximize);
	signalWindowEvent (maximizeEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::activate ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::isActive () const
{
	if(windowContext.topLevelWindow || hostedChildWindow != nullptr)
		return active ();
	else if(parentWindow)
		return parentWindow->isActive ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::onActivate (bool state)
{
	active (state);
	if(state)
		requestActivationToken ();
	
	if(windowMode == kWindowModeHosting)
	{
		if(state)
			activeChildWindow = this;
		else if(activeChildWindow == this)
			activeChildWindow = nullptr;
	}

	Window::onActivate (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::requestActivationToken ()
{
	#if WAYLAND_USE_XDG_ACTIVATION
	IWindow* topDialog = Desktop.getTopWindow (kDialogLayer);
	if(topDialog != nullptr && topDialog != this)
	{
		LinuxWindow* modalWindow = cast (unknown_cast<Window> (topDialog));
		if(modalWindow)
			modalWindow->requestActivationToken ();
		return;
	}

	if(windowContext.waylandSurface == nullptr)
		return;

	if(isInDestroyEvent ())
		return;

	activationToken.request (&windowListener, &windowListener);
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxWindow::isEnabled () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindow::close ()
{
	if(onClose ())
	{
		if(windowDecoration != nullptr)
			windowDecoration->close ();
		windowDecoration = nullptr;

		hide ();
		setInCloseEvent ();
		setInDestroyEvent (true);
		
		removed (nullptr);
		onDestroy ();
		setInCloseEvent (false);
		
		release ();
		
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::updateSize ()
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::resizeWindow (int edge)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API LinuxWindow::clientToScreen (Point& pos) const
{
	if(windowContext.topLevelWindow != nullptr)
		return pos;
	return pos.offset (size.left, size.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API LinuxWindow::screenToClient (Point& pos) const
{
	if(windowContext.topLevelWindow != nullptr)
		return pos;
	return pos.offset (-size.left, -size.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxWindow::setOpacity (float _opacity)
{
	opacity = _opacity;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindow::scrollClient (RectRef rect, PointRef delta)
{
	if(isInDestroyEvent ())
		return;

	if(collectUpdates)
	{
		// don't scroll, just invalidate
		Rect r (rect);
		r.offset (delta);
		r.join (rect);
		invalidate (r);
		return;
	}
	
	// inform render target
	if(NativeWindowRenderTarget* target = getRenderTarget ())
	{
		target->onScroll (rect, delta);
		finishScroll (rect, delta);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool LinuxWindow::suppressInput ()
{
	if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) || style.isCustomStyle (Styles::kWindowBehaviorTooltip))
		return false;
	
	if(getTitle () == CCL_SPY_NAME)
		return false;
	
	LinuxWindow* topWindow = cast (Desktop.getTopWindow (kDialogLayer));
	while(topWindow != nullptr && topWindow != this)
	{
		if(!topWindow->style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) && !topWindow->style.isCustomStyle (Styles::kWindowBehaviorTooltip))
			return true;
		topWindow = cast (ccl_cast<Window> (topWindow->getParentWindow ()));
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::setScaleFactor (int scaleFactor)
{
	setContentScaleFactor (scaleFactor);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::handleKeyboardEvent (const KeyEvent& event)
{
	if(inputEventsSuspended ())
	{
		suspendedKeyEvents.append (NEW KeyEvent (event));
		return;
	}

	SharedPtr<LinuxWindow> keeper (this);
		
	auto processEvent = [&] (const KeyEvent& event)
	{
		if(activeChildWindow && !activeChildWindow->suppressInput ())
		{
			// When hosting a Wayland client in a ChildWindow, host and client will both receive keyboard events independently.
			// There is currently no way to determine if a client handled a keyboard event.
			// To avoid handling the same keyboard event in host and client, we ignore all keyboard events if a client is in focus.
			#if 0
			bool handled = false;
			if(event.eventType == KeyEvent::kKeyDown)
				handled = activeChildWindow->onKeyDown (event);
			else if(event.eventType == KeyEvent::kKeyUp)
				handled = activeChildWindow->onKeyUp (event);

			if(handled)
				return;
			#else
			return;
			#endif
		}

		if(event.eventType == KeyEvent::kKeyDown)
			onKeyDown (event);
		else if(event.eventType == KeyEvent::kKeyUp)
			onKeyUp (event);
	};
	
	while(KeyEvent* event = suspendedKeyEvents.removeFirst ())
	{
		processEvent (*event);
		delete event;
	}
	processEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::handleFocus (const FocusEvent& event)
{
	SharedPtr<LinuxWindow> keeper (this);
	
	if(suppressInput ())
	{
		LinuxWindow* modalWindow = cast (Desktop.getTopWindow (kDialogLayer));
		if(modalWindow)
			modalWindow->requestActivationToken ();
	}
	else
		onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::handlePointerEvent (const InputHandler::PointerEvent& pointerEvent)
{
	SharedPtr<LinuxWindow> keeper (this);
	
	if(isInDestroyEvent ())
		return;

	if(!inputEventsSuspended ())
	{
		while(MouseEvent* event = suspendedEvents.removeFirst ())
		{
			switch(event->eventType)
			{
			case MouseEvent::kMouseUp :
				onMouseUp (*event);
				break;
			case MouseEvent::kMouseDown :
				onMouseDown (*event);
				break;
			}
			delete event;
		}
	}

	if(!popupStack.isEmpty ())
	{
		LinuxWindow* topMostPopup = popupStack.getLast ();
		if(topMostPopup != this && topMostPopup->getWaylandSurface () != pointerEvent.focus && topMostPopup->getWaylandSurface () != pointerEvent.oldSurface)
			topMostPopup->handlePointerEvent (pointerEvent);
	}
	
	KeyState keyState = 0;
	InputHandler::instance ().getActiveModifierKeys (keyState);
	keyState.keys |= pointerEvent.buttonState;
	
	double time = (pointerEvent.time == 0) ? System::GetProfileTime () : (pointerEvent.time / 1000.);
	Point where (wl_fixed_to_int (pointerEvent.x), wl_fixed_to_int (pointerEvent.y));
	
	if(get_flag<uint32_t> (pointerEvent.eventMask, InputHandler::kPointerEnter) && (pointerEvent.focus == getWaylandSurface () || hostedChildWindow != nullptr))
	{
		MouseEvent event (MouseEvent::kMouseEnter, where, keyState, time);
		if(!inputEventsSuspended ())
			onMouseEnter (event);
		mouseWindow = this;
		GUI.updateCursor ();
	}
	
	if(get_flag<uint32_t> (pointerEvent.eventMask, InputHandler::kPointerLeave) && (pointerEvent.oldSurface == getWaylandSurface () || hostedChildWindow != nullptr))
	{
		if(mouseWindow == this)
			mouseWindow = nullptr;
		MouseEvent event (MouseEvent::kMouseLeave, where, keyState, time);
		if(!inputEventsSuspended ())
			onMouseLeave (event);
	}
	
	bool shouldClose = false;
	
	const uint32_t axisMask = InputHandler::kPointerAxis | InputHandler::kPointerAxisDiscrete;
	if(pointerEvent.focus == getWaylandSurface () && !inputEventsSuspended () && get_flag<uint32_t> (pointerEvent.eventMask, axisMask))
	{
		for(int i = 0; i < ARRAY_COUNT (pointerEvent.axes); i++)
		{
			if(!pointerEvent.axes[i].valid)
				continue;
			
			int eventType = 0;
			if(wl_fixed_to_double (pointerEvent.axes[i].value) >= 0)
				eventType = i == 1 ? MouseWheelEvent::kWheelRight : MouseWheelEvent::kWheelDown;
			else
				eventType = i == 1 ? MouseWheelEvent::kWheelLeft : MouseWheelEvent::kWheelUp;
			
			MouseWheelEvent event (eventType, where, keyState, -wl_fixed_to_double (pointerEvent.axes[i].value));
			
			if(get_flag<uint32_t> (pointerEvent.eventMask, InputHandler::kPointerAxisDiscrete))
				event.delta = -pointerEvent.axes[i].discrete / 120.f;
			else if(get_flag<uint32_t> (pointerEvent.eventMask, InputHandler::kPointerAxis))
				event.wheelFlags |= MouseWheelEvent::kContinuous;
			
			// toggle axis
			if(event.keys.isSet (KeyState::kShift))
			{
				event.eventType = (event.eventType + 2) % 4;
				event.keys.keys &= ~KeyState::kShift;
				event.wheelFlags |= MouseWheelEvent::kAxisToggled;
			}
			
			if(pointerEvent.axes[i].inverted)
				event.wheelFlags |= MouseWheelEvent::kAxisInverted;
			
			onMouseWheel (event);
		}
	}

	if(get_flag<uint32_t> (pointerEvent.eventMask, InputHandler::kPointerButton))
	{
		if(pointerEvent.state == WL_POINTER_BUTTON_STATE_RELEASED)
		{
			if(pointerEvent.focus == getWaylandSurface () || (hostedChildWindow != nullptr && popupStack.isEmpty ()))
			{
				MouseEvent event (MouseEvent::kMouseUp, where, keyState, time);
				if(inputEventsSuspended ())
					suspendedEvents.append (NEW MouseEvent (event));
				else if(!isPositionReconfigured ())
					onMouseUp (event);
			}
			else if(dismissPopup && !inputEventsSuspended ())
			{
				shouldClose = true;
			}
		}
		else
		{
			if(pointerEvent.focus == getWaylandSurface () || (hostedChildWindow != nullptr && popupStack.isEmpty ()))
			{
				dismissPopup = false;
				isPositionReconfigured (false);

				MouseEvent event (MouseEvent::kMouseDown, where, keyState, time);
				if(inputEventsSuspended ())
					suspendedEvents.append (NEW MouseEvent (event));
				else
					onMouseDown (event);
				
				if(get_flag<uint32_t> (pointerEvent.buttonState, KeyState::kRButton) && popupStack.isEmpty ())
					popupContextMenu (where, false);
			}
			else if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
			{
				dismissPopup = true;
			}
			else if(pointerEvent.focus == nullptr && pointerEvent.oldSurface == nullptr)
				killFocusView ();
		}
	}

	if(get_flag<uint32_t> (pointerEvent.eventMask, InputHandler::kPointerMotion) && pointerEvent.focus == getWaylandSurface ())
	{
		if(!inputEventsSuspended ())
		{
			MouseEvent event (MouseEvent::kMouseMove, where, keyState, time);
			onMouseMove (event);
		}
		GUI.setMousePosition (clientToScreen (where));
	}
	
	if(shouldClose)
		close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::handleTouchEvent (const Linux::InputHandler::TouchEvent& touchEvent)
{
	SharedPtr<LinuxWindow> keeper (this);
	
	if(isInDestroyEvent ())
		return;

	if(!getTouchInputState ().getGestureManager ())
		getTouchInputState ().setGestureManager (NEW CustomGestureManager (*this));

	if(!popupStack.isEmpty ())
	{
		LinuxWindow* topMostPopup = popupStack.getLast ();
		if(topMostPopup != this && topMostPopup->getWaylandSurface () != touchEvent.focus)
			topMostPopup->handleTouchEvent (touchEvent);
	}
	
	double time = (touchEvent.time == 0) ? System::GetProfileTime () : (touchEvent.time / 1000.);
	Point where (wl_fixed_to_int (touchEvent.x), wl_fixed_to_int (touchEvent.y));

	if(touchEvent.type == InputHandler::TouchEventType::kTouchDown)
	{
		if(touchEvent.focus == getWaylandSurface ())
		{
			TouchInfo touchInfo (TouchEvent::EventType::kBegin, touchEvent.id, where, time); 
			getTouchInputState ().processTouch (touchInfo);
		}
	}

	if(touchEvent.type == InputHandler::TouchEventType::kTouchUp)
	{
		if(touchEvent.focus == getWaylandSurface ())
		{
			TouchInfo touchInfo (TouchEvent::EventType::kEnd, touchEvent.id, where, time);
			TouchInputState::TouchEventData data (TouchEvent::EventType::kEnd, KeyState (), TouchEvent::kTouchInput);
			getTouchInputState ().processTouchEnd (touchInfo, data);
		}
	}

	if(touchEvent.type == InputHandler::TouchEventType::kTouchMotion)
	{
		if(touchEvent.focus == getWaylandSurface ())
		{
			TouchInfo touchInfo (TouchEvent::EventType::kMove, touchEvent.id, where, time);
			getTouchInputState ().processTouch (touchInfo);
		}
	}
	
	if(touchEvent.type == InputHandler::TouchEventType::kTouchCancel)
	{
		if(touchEvent.focus == getWaylandSurface ())
		{
			TouchInfo touchInfo (TouchEvent::EventType::kCancel, touchEvent.id, where, time);
			getTouchInputState ().processTouch (touchInfo);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* LinuxWindow::getView ()
{
	return this;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::getParentContextRecursive (WindowContext& context, bool includeSelf) const
{
	context = {};
	const IWindow* parent = includeSelf ? this : parentWindow;
	while(parent != nullptr)
	{
		LinuxWindow* window = cast (unknown_cast<Window> (parent));
		const WindowContext* parentContext = window->getNativeContext ();
		if(parentContext)
		{
			if(context.waylandSurface == nullptr && parentContext->waylandSurface != nullptr)
				context.waylandSurface = parentContext->waylandSurface;
			if(context.topLevelWindow == nullptr && parentContext->topLevelWindow != nullptr)
				context.topLevelWindow = parentContext->topLevelWindow;
			if(context.xdgSurface == nullptr && parentContext->xdgSurface != nullptr)
				context.xdgSurface = parentContext->xdgSurface;
			
			if(context.topLevelWindow != nullptr && context.xdgSurface != nullptr)
				break;
		}
		parent = window ? window->parentWindow : nullptr;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxWindow* LinuxWindow::getTopLevelWindow () const
{
	IWindow* parent = parentWindow;
	while(parent != nullptr)
	{
		LinuxWindow* window = cast (unknown_cast<Window> (parent));
		const WindowContext* parentContext = window->getNativeContext ();
		if(parentContext && parentContext->topLevelWindow != nullptr)
			return window;
		parent = window ? window->parentWindow : nullptr;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point LinuxWindow::getPositioningOffset () const
{
	if(!windowContext.frameOffset.isNull ())
		return windowContext.frameOffset;
	
	IWindow* parent = parentWindow;
	while(parent != nullptr)
	{
		LinuxWindow* window = cast (unknown_cast<Window> (parent));
		const WindowContext* parentContext = window->getNativeContext ();
		if(window && parentContext)
		{
			if(!parentContext->frameOffset.isNull ())
			{
				return parentContext->frameOffset;
			}
			else if(parentContext->xdgSurface != nullptr)
			{
				Rect frameSize;
				window->getFrameSize (frameSize);
				return Point (-frameSize.left, -frameSize.top);
			}
		}
		parent = window ? cast (window)->parentWindow : nullptr;
	}

	return Point ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point LinuxWindow::getFrameOffset () const
{
	Rect frameSize;
	getFrameSize (frameSize);
	Rect size = getSize ();
	return Point (size.left - frameSize.left, size.top - frameSize.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxWindow* LinuxWindow::getParentLinuxWindow () const
{
	return cast (unknown_cast<Window> (parentWindow));
}

//************************************************************************************************
// LinuxWindow::WindowListener
//************************************************************************************************

LinuxWindow::WindowListener::WindowListener (LinuxWindow& window)
: window (window),
  nextDecorationMode (0)
{
	xdg_surface_listener::configure = onConfigureSurface;
	xdg_toplevel_listener::configure = onTopLevelConfigure;
	xdg_toplevel_listener::close = onClose;
	xdg_toplevel_listener::configure_bounds = onConfigureBounds;
	#ifdef XDG_TOPLEVEL_WM_CAPABILITIES_SINCE_VERSION
	xdg_toplevel_listener::wm_capabilities = onWindowManagerCapabilities;
	#endif
	xdg_popup_listener::configure = onPopupConfigure;
	xdg_popup_listener::popup_done = onPopupDone;
	xdg_popup_listener::repositioned = onPopupRepositioned;
	zxdg_toplevel_decoration_v1_listener::configure = onConfigureDecoration;
	#if WAYLAND_USE_XDG_ACTIVATION
	xdg_activation_token_v1_listener::done = onActivationDone;
	#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onConfigureSurface (void* data, xdg_surface* surface, uint32_t serial)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	
	xdg_surface_ack_configure (surface, serial);
	
	if(This->nextDecorationMode != 0)
	{
		bool serverSideDecoration = (This->nextDecorationMode == ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		This->window.enableClientSideDecoration (!serverSideDecoration);
		This->nextDecorationMode = 0;
	}
	
	if(!This->window.isConfigured ())
	{
		This->window.isConfigured (true);
		
		Rect size (This->window.getSize ());
		This->window.setWindowSize (size);
	}
	
	This->window.updateSizeLimits ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onTopLevelConfigure (void* data, xdg_toplevel* toplevel, int32_t width, int32_t height, wl_array* states)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	if(This->window.isInDestroyEvent ())
		return;
	
	if(This->window.suppressInput ())
		return;

	bool active = false;
	bool maximized = false;
	bool fullscreen = false;
	WaylandArrayForEach (const uint32_t*, state, states)
	{
		if(state)
		{
			switch(*state)
			{
			case XDG_TOPLEVEL_STATE_MAXIMIZED :
				maximized = true;
				break;
			case XDG_TOPLEVEL_STATE_FULLSCREEN :
				fullscreen = true;
				break;
			case XDG_TOPLEVEL_STATE_RESIZING :
				break;
			case XDG_TOPLEVEL_STATE_ACTIVATED :
				active = true;
				break;
			case XDG_TOPLEVEL_STATE_TILED_LEFT :
				break;
			case XDG_TOPLEVEL_STATE_TILED_RIGHT :
				break;
			case XDG_TOPLEVEL_STATE_TILED_TOP :
				break;
			case XDG_TOPLEVEL_STATE_TILED_BOTTOM :
				break;
			}
		}
	}
	if(This->window.isMaximized () != maximized)
		This->window.onMaximize (maximized);
	This->window.fullscreen (fullscreen);
	if(This->window.isActive () != active)
		This->window.onActivate (active);

	// width and height provided here include all subsurfaces, subtract frame offset to get the size of the client area
	if(This->window.style.isCustomStyle (Styles::kWindowBehaviorSizable) && width > 0 && height > 0)
	{
		Rect totalFrameSize;
		This->window.getFrameSize (totalFrameSize);
		Rect clientSize = This->window.getSize ();
		Rect size (0, 0, width - totalFrameSize.getWidth () + clientSize.getWidth (), height - totalFrameSize.getHeight () + clientSize.getHeight ());
		size.moveTo (clientSize.getLeftTop ());
		
		if(clientSize != size)
			This->window.applySize (size);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onClose (void* data, xdg_toplevel* toplevel)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	
	if(This->window.suppressInput ())
		return;
	
	if(This->window.windowContext.topLevelWindow == toplevel)
		This->window.deferClose ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onConfigureBounds (void* data, xdg_toplevel* toplevel, int32_t width, int32_t height)
{
	WindowListener* This = static_cast<WindowListener*> (data);

	MonitorHelper::instance ().setWorkAreaSize (Point (width, height));

	if((!This->window.sizeRestored () && This->window.style.isCustomStyle (Styles::kWindowBehaviorInflate))
		|| (This->window.size.getWidth () > width || This->window.size.getHeight () > height))
	{
		This->window.inflate ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onWindowManagerCapabilities (void *data, xdg_toplevel* xdg_toplevel, wl_array* capabilities)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onPopupConfigure (void* data, xdg_popup* popup, int32_t x, int32_t y, int32_t width, int32_t height)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	if(This->window.isInDestroyEvent ())
		return;
	
	if(This->window.suppressInput ())
		return;
	
	if(width == 0 || height == 0)
		return;
	
	Rect size (x, y, x + width, y + height);
	Point offset = This->window.getPositioningOffset ();
	size.offset (-offset.x, -offset.y);
	if(!This->window.style.isCustomStyle (Styles::kWindowBehaviorSizable))
	{
		size.setWidth (This->window.getWidth ());
		size.setHeight (This->window.getHeight ());
	}
	if(This->window.getSize ().getLeftTop () != size.getLeftTop ())
		This->window.isPositionReconfigured (true);
	This->window.setUserSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onPopupDone (void *data, xdg_popup* popup)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	
	if(This->window.windowContext.popupWindow == popup)
	{
		if(This->window.getStyle ().isCustomStyle (Styles::kWindowBehaviorTooltip))
			This->window.hide ();
		else
			This->window.close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onPopupRepositioned (void* data, xdg_popup* popup, uint32_t token)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindow::WindowListener::onConfigureDecoration (void* data, zxdg_toplevel_decoration_v1* decoration, uint32_t mode)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	This->nextDecorationMode = mode;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

#if WAYLAND_USE_XDG_ACTIVATION
void LinuxWindow::WindowListener::onActivationDone (void* data, xdg_activation_token_v1* token, CStringPtr tokenString)
{
	WindowListener* This = static_cast<WindowListener*> (data);
	
	if(tokenString != nullptr)
		This->window.activate (tokenString);
}
#endif

//************************************************************************************************
// SubSurfaceWindow
//************************************************************************************************

SubSurfaceWindow::SubSurfaceWindow (LinuxWindow& parent)
: SubSurface<LinuxWindow> (parent)
{
	handle = &windowContext;
	parentWindow = &parent;
	
	wantsFrameCallback (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SubSurfaceWindow::showWindow (bool state)
{
	if(state && windowContext.waylandSurface == nullptr)
	{
		WaylandClient& client = WaylandClient::instance ();
	
		wl_compositor* compositor = client.getCompositor ();
		wl_display* display = client.getDisplay ();
		if(compositor == nullptr || display == nullptr)
			return;
		
		createSurface ();
		setSynchronous (false);
		setPosition (Point (size.left, size.top));
		
		windowContext.waylandSurface = getWaylandSurface ();
		if(windowContext.waylandSurface)
			wl_surface_set_input_region (windowContext.waylandSurface, nullptr);
		
		enableInput ();
	}
	else if(!state && windowContext.waylandSurface != nullptr)
	{
		if(mouseWindow == this)
			mouseWindow = nullptr;
		
		enableInput (false);
			
		windowContext.waylandSurface = nullptr;
		
		isConfigured (false);
		safe_release (renderTarget);
		destroySurface ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SubSurfaceWindow::moveWindow (PointRef pos)
{
	setPosition (pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SubSurfaceWindow::isVisible () const
{
	return parentWindow->isVisible ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API SubSurfaceWindow::isActive () const
{
	return parentWindow->isActive ();
}
	
//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API SubSurfaceWindow::clientToScreen (Point& pos) const
{
	static_cast<LinuxWindow&> (parent).clientToScreen (pos);
	return pos.offset (size.left, size.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API SubSurfaceWindow::screenToClient (Point& pos) const
{
	static_cast<LinuxWindow&> (parent).screenToClient (pos);
	return pos.offset (-size.left, -size.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API SubSurfaceWindow::invalidate (RectRef rect)
{
	if(!isConfigured () || isInDestroyEvent ())
		return;
	
	LinuxWindow::invalidate (rect);
	// This window's render target does not receive frame callbacks, so make sure that the parent requests a new frame.
	static_cast<LinuxWindow&> (parent).invalidate (Rect ());
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SubSurfaceWindow::handleKeyboardEvent (const KeyEvent& event)
{
	LinuxWindow::handleKeyboardEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SubSurfaceWindow::handleFocus (const FocusEvent& event)
{
	LinuxWindow::handleFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void SubSurfaceWindow::handlePointerEvent (const InputHandler::PointerEvent& event)
{
	LinuxWindow::handlePointerEvent (event);
}
	
//************************************************************************************************
// LinuxWindowDecoration
//************************************************************************************************

LinuxWindowDecoration::LinuxWindowDecoration (LinuxWindow& parent)
: SubSurfaceWindow (parent),
  currentCursorId (-1)
{
	if(parent.getStyle ().isCommonStyle (Styles::kTranslucent))
	{
		style.setCommonStyle (Styles::kTranslucent);
		style.setCommonStyle (Styles::kTransparent);
	}
	
	controller.canMinimize (parent.getWindowContext ().topLevelWindow != nullptr);
	controller.canMaximize (parent.getStyle ().isCustomStyle (Styles::kWindowBehaviorMaximizable));
	controller.canClose (!parent.getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame));
	controller.setBorderWidth (0);

	updateTitleBarHeight ();
	
	controller.attach (&parent);
	View* view = controller.getDecorationView ();
	if(view)
	{
		addView (view);
		view->retain ();
		view->setSize (getSize ());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

LinuxWindowDecoration::~LinuxWindowDecoration ()
{
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecoration::showWindow (bool state)
{
	if(state && windowContext.waylandSurface == nullptr)
	{
		SignalSource::addObserver (Signals::kGUI, this);
	}
	else if(!state && windowContext.waylandSurface != nullptr)
	{
		SignalSource::removeObserver (Signals::kGUI, this);
	}
	
	SubSurfaceWindow::showWindow (state);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API LinuxWindowDecoration::notify (ISubject* subject, MessageRef msg)
{
	if(msg == Signals::kSystemMetricsChanged)
		updateTitleBarHeight ();
	
	SubSurfaceWindow::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecoration::applySize (RectRef size)
{
	SubSurfaceWindow::applySize (size);
	
	WaylandClient& context = WaylandClient::instance ();
	wl_region* region = wl_compositor_create_region (context.getCompositor ());
	if(region)
	{
		wl_region_add (region, 0, 0, size.getWidth (), ccl_max (controller.getTitleBarHeight (), kResizeBorderRadius));
		wl_region_add (region, 0, 0, kResizeBorderRadius, size.getHeight ());
		wl_region_add (region, size.getWidth () - kResizeBorderRadius, 0, kResizeBorderRadius, size.getHeight ());
		wl_region_add (region, 0, size.getHeight () - kResizeBorderRadius, size.getWidth (), kResizeBorderRadius);
		wl_surface_set_input_region (windowContext.waylandSurface, region);
		wl_region_destroy (region);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecoration::updateTitleBarHeight ()
{
	LinuxWindow& parentWindow = static_cast<LinuxWindow&> (parent);
	if(parentWindow.getStyle ().isCustomStyle (Styles::kWindowAppearanceCustomFrame))
	{
		controller.setTitleBarHeight (0);
		update ();
		return;
	}
	
	int titleBarHeight = 0;
	if(NativeThemePainter::instance ().getSystemMetric (titleBarHeight, ThemeElements::kTitleBarHeight))
	{
		controller.setTitleBarHeight (titleBarHeight);
		update ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API LinuxWindowDecoration::close ()
{
	removeAll ();
	controller.attach (nullptr);
	return SubSurfaceWindow::close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecoration::handlePointerEvent (const InputHandler::PointerEvent& event)
{
	LinuxWindow::handlePointerEvent (event);
	
	LinuxWindow& parentWindow = static_cast<LinuxWindow&> (parent);
	
	if(parentWindow.getStyle ().isCustomStyle (Styles::kWindowBehaviorSizable))
		handleResize (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecoration::handleResize (const InputHandler::PointerEvent& event)
{
	WaylandClient& context = WaylandClient::instance ();
	LinuxWindow& parentWindow = static_cast<LinuxWindow&> (parent);
	
	Point where (wl_fixed_to_int (event.x), wl_fixed_to_int (event.y));
	
	bool left = where.x < kResizeBorderRadius;
	bool right = where.x >= size.getWidth () - kResizeBorderRadius;
	bool top = where.y < kResizeBorderRadius;
	bool bottom = where.y >= size.getHeight () - kResizeBorderRadius;
	
	bool leftTop = (left || top) && where.x < kResizeCornerRadius && where.y < kResizeCornerRadius;
	bool leftBottom = (left || bottom) && where.x < kResizeCornerRadius && where.y >= size.getHeight () - kResizeCornerRadius;
	bool rightTop = (right || top) && where.x >= size.getWidth () - kResizeCornerRadius && where.y < kResizeCornerRadius;
	bool rightBottom = (right || bottom) && where.x >= size.getWidth () - kResizeCornerRadius && where.y >= size.getHeight () - kResizeCornerRadius;
	
	if(event.focus == getWaylandSurface ())
	{
		ThemeCursorID cursorId = -1;
		uint32_t edge = XDG_TOPLEVEL_RESIZE_EDGE_NONE;
		if(leftTop)
		{
			cursorId = ThemeElements::kSizeLeftUpCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_LEFT;
		}
		else if(leftBottom)
		{
			cursorId = ThemeElements::kSizeLeftDownCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_LEFT;
		}
		else if(rightTop)
		{
			cursorId = ThemeElements::kSizeRightUpCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP_RIGHT;
		}
		else if(rightBottom)
		{
			cursorId = ThemeElements::kSizeRightDownCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM_RIGHT;
		}
		else if(top)
		{
			cursorId = ThemeElements::kSizeUpCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_TOP;
		}
		else if(left)
		{
			cursorId = ThemeElements::kSizeLeftCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_LEFT;
		}
		else if(bottom)
		{
			cursorId = ThemeElements::kSizeDownCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_BOTTOM;
		}
		else if(right)
		{
			cursorId = ThemeElements::kSizeRightCursor;
			edge = XDG_TOPLEVEL_RESIZE_EDGE_RIGHT;
		}
		
		if((event.eventMask & (InputHandler::kPointerEnter | InputHandler::kPointerMotion)) != 0
			&& cursorId != currentCursorId)
		{
			AutoPtr<MouseCursor> cursor = cursorId >= 0 ? MouseCursor::createCursor (cursorId) : nullptr;
			setCursor (cursor);
			currentCursorId = cursorId;
		}

		if((top || left || bottom || right)
			&& get_flag<uint32_t> (event.eventMask, InputHandler::kPointerButton)
			&& get_flag<uint32_t> (event.buttonState, KeyState::kLButton)
			&& event.state == WL_POINTER_BUTTON_STATE_PRESSED
			&& parentWindow.getWindowContext ().topLevelWindow != nullptr)
		{
			xdg_toplevel_resize (parentWindow.getWindowContext ().topLevelWindow, context.getSeat (), event.serial, edge);
		}
	}
	
	if(get_flag<uint32_t> (event.eventMask, InputHandler::kPointerLeave))
	{
		GUI.resetCursor ();
		currentCursorId = -1;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord LinuxWindowDecoration::getBorderWidth () const
{
	return controller.getBorderWidth ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Coord LinuxWindowDecoration::getTitleBarHeight () const
{
	return controller.getTitleBarHeight ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecoration::update ()
{
	controller.updateDecoration ();
}

//************************************************************************************************
// LinuxWindowDecorationController
//************************************************************************************************

void LinuxWindowDecorationController::onMinimize ()
{
	if(targetWindow)
		LinuxWindow::cast (targetWindow)->minimize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void LinuxWindowDecorationController::onShowMenu ()
{
	if(targetWindow)
		LinuxWindow::cast (targetWindow)->showMenu ();
}
