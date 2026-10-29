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
// Filename    : ccl/platform/wasm/gui/window.wasm.cpp
// Description : Platform-specific Window implementation
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/platform/wasm/gui/window.wasm.h"
#include "ccl/platform/wasm/gui/keyevent.wasm.h"
#include "ccl/platform/wasm/gui/systemevent.wasm.h"
#include "ccl/platform/wasm/gui/dragndrop.wasm.h"

#include "ccl/base/asyncoperation.h"
#include "ccl/base/message.h"

#include "ccl/gui/gui.h"
#include "ccl/gui/windows/desktop.h"
#include "ccl/gui/windows/dialog.h"
#include "ccl/gui/popup/popupselector.h"
#include "ccl/gui/graphics/nativegraphics.h"
#include "ccl/gui/touch/touchcollection.h"
#include "ccl/gui/touch/gesturemanager.h"

#include "ccl/public/systemservices.h"

#include <emscripten/html5.h>

namespace CCL {
namespace Wasm {

//////////////////////////////////////////////////////////////////////////////////////////////////
// JS Helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, createWindowElement, (CStringPtr id),
{
	var frame = document.createElement ('div');
	frame.id = 'frame' + UTF8ToString (id);
	frame.setAttribute ("tabindex", "0"); // set a tab index to make this element focusable
	frame.setAttribute ("class", "window");
	frame.setAttribute ("oncontextmenu", "event.preventDefault ()");

	var canvas = document.createElement ('canvas');
	canvas.id = 'content' + UTF8ToString (id);
	canvas.setAttribute ("class", "windowContent");
	canvas.setAttribute ("oncontextmenu", "event.preventDefault ()");

	frame.appendChild (canvas);
	document.body.appendChild (frame);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, removeWindowElement, (CStringPtr id),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element)
		element.remove ();
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, moveWindowElement, (CStringPtr id, int x, int y),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element)
	{
		element.style.position = 'fixed';
		element.style.left = x + 'px';
		element.style.top = y + 'px';
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, addWindowClass, (CStringPtr id, CStringPtr classId),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element)
		element.classList.add (UTF8ToString (classId));
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, enableWindowResize, (CStringPtr id, bool state),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element && CCL.resizeObserver)
	{
		if(state)
		{
			CCL.resizeObserver.observe (element);
			element.style.resize = 'both';
		}
		else
		{
			CCL.resizeObserver.unobserve (element);
			element.style.resize = 'none';
		}
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, maximizeWindowElement, (CStringPtr id),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element)
	{
		element.style.position = 'absolute';
		element.style.left = '0px';
		element.style.top = '0px';
		element.style.width = '100%';
		element.style.height = '100%';
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, createWindowTitleBar, (CStringPtr id, CStringPtr title),
{
	var frame = CCL.getFrameElement (UTF8ToString (id));
	var content = CCL.getContentElement (UTF8ToString (id));
	if(frame && content)
	{
		var titleBar = document.createElement ('div');
		titleBar.id = 'titleBar' + UTF8ToString (id);
		titleBar.setAttribute ("class", "titleBar");

		var titleSpan = document.createElement ('span');
		titleSpan.id = 'title' + UTF8ToString (id);
		titleSpan.textContent = UTF8ToString (title);
		
		var titleButtons = document.createElement ('div');
		titleButtons.id = 'titleButtons' + UTF8ToString (id);
		titleButtons.setAttribute ("class", "titleButtons");

		var closeButton = document.createElement ('button');
		closeButton.id = 'close' + UTF8ToString (id);
		closeButton.setAttribute ("class", "closeButton");

		titleButtons.appendChild (closeButton);
		titleBar.appendChild (titleSpan);
		titleBar.appendChild (titleButtons);
		frame.insertBefore (titleBar, content)
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, addMaximizeButton, (CStringPtr id),
{
	var titleButtons = document.getElementById ('titleButtons' + UTF8ToString (id));
	var closeButton = document.getElementById ('close' + UTF8ToString (id));
	if(titleButtons && closeButton)
	{
		var maximizeButton = document.createElement ('button');
		maximizeButton.id = 'maximize' + UTF8ToString (id);
		maximizeButton.setAttribute ("class", "maximizeButton");

		titleButtons.insertBefore (maximizeButton, closeButton);
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, updateWindowTitleBar, (CStringPtr id, CStringPtr title),
{
	var titleSpan = document.getElementById ('title' + UTF8ToString (id));
	if(titleSpan)
		titleSpan.textContent = UTF8ToString (title);
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (int, getWindowTitleBarHeight, (CStringPtr id),
{
	var titleBar = document.getElementById ('titleBar' + UTF8ToString (id));
	if(titleBar)
		return titleBar.offsetHeight;
	return 0;
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, enablePopover, (CStringPtr id),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	if(element)
	{
		// show the popup after mouse up and click events have been processed
		setTimeout (() => 
		{
			requestAnimationFrame (() => 
			{
				element.setAttribute ("popover", "auto");
				element.showPopover ();
			});
		}, 50);
		
		element.addEventListener ('toggle', (e) => 
		{
			CCL.onJSEvent ('toggle', UTF8ToString (id), e.newState);
		});
	}
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, setFocus, (CStringPtr id),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	var inputMethod = document.getElementById ('inputmethod');
	if(element && !inputMethod)
		element.focus ();
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, enableDropHandler, (CStringPtr id),
{
	var element = CCL.getFrameElement (UTF8ToString (id));

	const setDropEffect = (dataTransfer, result) =>
	{
		dataTransfer.dropEffect = "none";
		if(result & CCL.Constants.kDragSessionDropResultMove)
			dataTransfer.dropEffect = "move";
		if(result & CCL.Constants.kDragSessionDropResultCopyReal)
			dataTransfer.dropEffect = "copy";
		if(result & CCL.Constants.kDragSessionDropResultCopyShared)
			dataTransfer.dropEffect = "link";
	};

	element.addEventListener ('dragenter', (e) =>
	{
		e.preventDefault ();
		CCL.onJSEvent ('dragenter-effect', UTF8ToString (id), e.dataTransfer.effectAllowed);
		var i = 0;
		for(const item of e.dataTransfer.items)
		{
			const index = i;
			if(item.kind === 'string')
			{
				CCL.onJSEvent ('dragenter-mime', UTF8ToString (id), index, item.type);
				item.getAsString ((s) =>
				{
					CCL.onJSEvent ('dragenter-data', UTF8ToString (id), index, s);
					const result = Number (CCL.onJSEvent ('dragenter', UTF8ToString (id), e.clientX, e.clientY));
					setDropEffect (e.dataTransfer, result);
				});
			}
			else if(item.kind === 'file')
			{
				CCL.onJSEvent ('dragenter-mime', UTF8ToString (id), index, 'text/uri-list');
				const fileUrl = 'file:///tmp/drop/' + index;
				CCL.onJSEvent ('dragenter-data', UTF8ToString (id), index, fileUrl);
				const result = Number (CCL.onJSEvent ('dragenter', UTF8ToString (id), e.clientX, e.clientY));
				setDropEffect (e.dataTransfer, result);
			}
			i++;
		}
		const result = Number (CCL.onJSEvent ('dragenter', UTF8ToString (id), e.clientX, e.clientY));
		setDropEffect (e.dataTransfer, result);
	});
	element.addEventListener ('dragover', (e) =>
	{
		e.preventDefault ();
		const result = Number (CCL.onJSEvent ('dragover', UTF8ToString (id), e.clientX, e.clientY));
		setDropEffect (e.dataTransfer, result);
	});
	element.addEventListener ('dragleave', (e) =>
	{
		e.preventDefault ();
		CCL.onJSEvent ('dragleave', UTF8ToString (id));
	});
	element.addEventListener ('drop', (e) =>
	{
		e.preventDefault ();
		var pending = 0;
		var i = 0;

		for(const item of e.dataTransfer.items)
		{
			const index = i;
			if(item.kind === 'string')
			{
				pending++;
				item.getAsString ((s) =>
				{
					CCL.onJSEvent ('drop-data', UTF8ToString (id), index, s);
					pending--;
					if(pending == 0)
						CCL.onJSEvent ('drop', UTF8ToString (id), e.clientX, e.clientY);
				});
			}
			else if(item.kind === 'file')
			{
				pending++;
				const file = item.getAsFile ();
				if(file)
				{
					const dragDir = '/tmp/drop/';
					CCL.makeDirectory (dragDir);
					const path = dragDir + file.name;

					CCL.readFileToFS (file, path)
						.then (() =>
						{
							CCL.onJSEvent ('drop-data', UTF8ToString (id), index, 'file://' + path);
							pending--;
							if(pending == 0)
								CCL.onJSEvent ('drop', UTF8ToString (id), e.clientX, e.clientY);
						})
						.catch (() =>
						{
							pending--;
							if(pending == 0)
								CCL.onJSEvent ('drop', UTF8ToString (id), e.clientX, e.clientY);
						});
				}
				else
				{
					pending--;
					if(pending == 0)
						CCL.onJSEvent ('drop', UTF8ToString (id), e.clientX, e.clientY);
				}
			}
			i++;
		}

		// If no items required async processing, send drop event immediately
		if(pending == 0)
			CCL.onJSEvent ('drop', UTF8ToString (id), e.clientX, e.clientY);

		CCL.currentDataTransfer = null;
	});
});

//////////////////////////////////////////////////////////////////////////////////////////////////

EM_JS (void, enableDragHandler, (CStringPtr id),
{
	var element = CCL.getFrameElement (UTF8ToString (id));
	element.draggable = true;
	element.addEventListener ('dragstart', (e) =>
	{
		CCL.currentDataTransfer = e.dataTransfer;
		if(CCL.onJSEvent ('dragstart', UTF8ToString (id), e.targetX, e.targetY) == false)
			e.preventDefault ();
		e.dataTransfer = CCL.currentDataTransfer;
	});
	element.addEventListener ('drag', (e) =>
	{
		e.dataTransfer = CCL.currentDataTransfer;
	});
});

} // namespace Wasm
} // namespace CCL

using namespace CCL;
using namespace Wasm;

//************************************************************************************************
// PopupSelectorWindow
//************************************************************************************************

void PopupSelectorWindow::onActivate (bool state)
{
	SuperClass::onActivate (state);
}

//************************************************************************************************
// Dialog
//************************************************************************************************

IAsyncOperation* Dialog::showPlatformDialog (IWindow* parent)
{
	makeNativePopupWindow (parent);
	
	attached (this);
	show ();
	
	AsyncOperation* operation = NEW AsyncOperation;
	operation->setState (AsyncOperation::kStarted);
	setDialogOperation (operation);
	return operation;
}

//************************************************************************************************
// WasmDialog
//************************************************************************************************

WasmDialog::WasmDialog (const Rect& size, StyleRef style, StringRef title)
: WasmWindow (size, style, title)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmDialog::close ()
{
	if(isInCloseEvent ())
		return true;

	if(onClose ())
	{
		SharedPtr<WasmDialog> guard (this);

		if(dialogOperation)
		{
			dialogOperation->setResult (static_cast<Dialog*> (this)->getDialogResult ());
			dialogOperation->setState (AsyncOperation::kCompleted);
		}

		hide ();
		setInCloseEvent ();
		setInDestroyEvent (true);
		
		removed (nullptr);
		onDestroy ();
		setInCloseEvent (false);
		
		Desktop.removeWindow (this);

		return true;
	}
	return false;
}

//************************************************************************************************
// WasmWindow
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (WasmWindow, Window)

/////////////////////////////////////////////////////////////////////////////////////////////////

WasmWindow::WasmWindow (const Rect& size, StyleRef style, StringRef title)
: Window (size, style, title),
  parentWindow (nullptr),
  stateFlags (0),
  savedDpiFactor (emscripten_get_device_pixel_ratio ()),
  dragEffect (IDragSession::kDropNone),
  dragSession (nullptr),
  mouseDownEventSent (true)
{
	UID uid;
	uid.generate ();
	uid.toCString (id, UID::kCompact);

	SystemEventDelegate::instance ().registerEventHandler (id, *this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WasmWindow::~WasmWindow ()
{
	ASSERT (dragSession == nullptr)
	safe_release (dragSession);

	SystemEventDelegate::instance ().unregisterEventHandler (*this);

	destruct ();
	cancelSignals ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::getFrameId (MutableCString& frameId) const
{
	frameId = "#frame";
	frameId.append (id);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::getContentId (MutableCString& contentId) const
{
	contentId = "#content";
	contentId.append (id);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::makeNativePopupWindow (IWindow* parent)
{
	handle = (void*)id.str ();
	parentWindow = parent;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::makeNativeChildWindow (void* nativeParent)
{
	handle = (void*)id.str ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API WasmWindow::clientToScreen (Point& pos) const
{
	return pos.offset (size.left, size.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Point& CCL_API WasmWindow::screenToClient (Point& pos) const
{
	return pos.offset (-size.left, -size.top);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::setWindowSize (Rect& size)
{
	moveWindow (size.getLeftTop ());
	setUserSize (size);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::setUserSize (RectRef _size)
{
	Rect newSize (_size);
	constrainSize (newSize);
	
	if(newSize.getWidth () == 0)
		newSize.setWidth (1);
		
	if(newSize.getHeight () == 0)
		newSize.setHeight (1);
	
	View::setSize (getSizeLimits ().makeValid (newSize));

	Rect r;
	getClientRect (r);
	invalidate (r);
	
	if(isConfigured ())
		if(NativeWindowRenderTarget* target = getRenderTarget ())
			target->onSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::getUserSize (Rect& userSize) const
{
	userSize = getSize ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::getFrameSize (Rect& size) const
{
	Rect totalFrameSize = getSize ();
	totalFrameSize.bottom += getTitleBarHeight ();
	size = totalFrameSize;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::setFrameSize (RectRef size)
{
	Rect newUserSize = size;
	newUserSize.bottom -= getTitleBarHeight ();
	setUserSize (newUserSize);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WasmWindow::getTitleBarHeight () const
{
	if(style.isCustomStyle (Styles::kWindowAppearanceTitleBar))	
		return getWindowTitleBarHeight (id);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::maximize (tbool state)
{
	maximized (state);
	if(state)
	{
		savedSize = getSize ();
		maximizeWindowElement (id);
		emscripten_set_resize_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onPageResized);
		Rect rect;
		Desktop.getMonitorSize (rect, 0, false);
		setFrameSize (rect);
	}
	else
	{
		if(!savedSize.isEmpty ())
		{
			setWindowSize (savedSize);
			savedSize.setEmpty ();
		}
		emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_RESIZE, (void*)onPageResized);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmWindow::isMaximized () const
{
	return maximized ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::resizeWindow (int edge)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::moveWindow (PointRef pos)
{
	moveWindowElement (id, pos.x, pos.y);
	size.moveTo (pos);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::moveWindow ()
{
	if(isMaximized ())
		return;
	if(isResizingWindow ())
		return;
	GUI.getMousePosition (moveAnchor);
	screenToClient (moveAnchor);
	isMoving (true);
	emscripten_set_mousemove_callback (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, false, onMoveWindowEvent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::stopMoveWindow ()
{
	emscripten_html5_remove_event_listener (EMSCRIPTEN_EVENT_TARGET_WINDOW, this, EMSCRIPTEN_EVENT_MOUSEMOVE, (void*)onMoveWindowEvent);
	isMoving (false);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onMoveWindow (int x, int y)
{
	if(isMoving ())
	{
		Point where (x, y);
		screenToClient (where);

		Point position = size.getLeftTop ();
		position.offset (where.x - moveAnchor.x, where.y - moveAnchor.y);
		moveWindow (position);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::center ()
{
	if(isMaximized ())
		return;

	if(layer == kDialogLayer)
	{
		if(Window* appWindow = unknown_cast<Window> (Desktop.getApplicationWindow ()))
		{
			Rect r;
			Point p;
			appWindow->getFrameSize (r);
			appWindow->getPosition (p);
			r.moveTo (p);

			Rect r2 (0, 0, getWidth (), getHeight ());
			r2.center (r);
			moveWindow (r2.getLeftTop ());
			return;
		}
	}

	Rect rect;
	Desktop.getMonitorSize (rect, 0, false);
	moveWindow (Point (rect.left + rect.getWidth () / 2 - size.getWidth () / 2, rect.top + rect.getHeight () / 2 - size.getHeight () / 2));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

float CCL_API WasmWindow::getContentScaleFactor () const
{
	return savedDpiFactor;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmWindow::setContentScaleFactor (float factor)
{
	onDpiChanged (factor);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDpiChanged (float dpiFactor)
{
	if(dpiFactor != savedDpiFactor)
	{
		onDisplayPropertiesChanged (DisplayChangedEvent (dpiFactor, DisplayChangedEvent::kResolutionChanged));
		
		Rect r;
		getClientRect (r);
		invalidate (r);
			
		savedDpiFactor = dpiFactor;

		if(isConfigured ())
		{
			if(NativeWindowRenderTarget* target = getRenderTarget ())
				target->onSize ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onResizeEvent ()
{
	if(!isConfigured ())
		return;
	if(isMaximized ())
		return;
	if(!style.isCustomStyle (Styles::kWindowBehaviorSizable))
		return;
	
	MutableCString contentId;
	getContentId (contentId);
	double width = 0.;
	double height = 0.;
	if(emscripten_get_element_css_size (contentId, &width, &height) == EMSCRIPTEN_RESULT_SUCCESS)
	{
		isResizingWindow (true);

		if(width > 0. && height > 0.)
		{
			Rect newSize = getSize ();
			newSize.setWidth (Coord(width));
			newSize.setHeight (Coord(height));
			setUserSize (newSize);

			if(isConfigured ())
				if(NativeWindowRenderTarget* target = getRenderTarget ())
					target->onRender ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::setWindowTitle (StringRef title)
{
	if(isEqualUnknown (Desktop.getApplicationWindow (), asUnknown ()))
		emscripten_set_window_title (MutableCString (title, Text::kUTF8));
	else
		updateWindowTitleBar (id, MutableCString (title, Text::kUTF8));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::showWindow (bool state, bool _activate)
{
	if(state && !isConfigured ())
	{
		createWindowElement (id);
		moveWindow (size.getLeftTop ());

		if(style.isCustomStyle (Styles::kWindowBehaviorProgressDialog))
			addWindowClass (id, "progressDialog");

		if(style.isCustomStyle (Styles::kWindowBehaviorTopMost))
			addWindowClass (id, "topmost");

		if(style.isCustomStyle (Styles::kWindowAppearanceDropShadow))
			addWindowClass (id, "dropShadow");

		if(style.isCustomStyle (Styles::kWindowAppearanceTitleBar) && !style.isCustomStyle (Styles::kWindowAppearanceCustomFrame))
		{
			addWindowClass (id, "titleBar");
			createWindowTitleBar (id, MutableCString (getTitle (), Text::kUTF8));

			MutableCString closeButtonId = "#close";
			closeButtonId.append (id);
			emscripten_set_click_callback (closeButtonId, this, false, onCloseEvent);

			if(style.isCustomStyle (Styles::kWindowBehaviorMaximizable))
			{
				addMaximizeButton (id);
				MutableCString maximizeButtonId = "#maximize";
				maximizeButtonId.append (id);
				emscripten_set_click_callback (maximizeButtonId, this, false, onMaximizeEvent);
			}
		}

		if(style.isCustomStyle (Styles::kWindowBehaviorSizable))
			enableWindowResize (id, true);

		if(NativeWindowRenderTarget* target = getRenderTarget ())
			target->onSize ();

		enableInput (true);

		isConfigured (true);

		setFocus (id);

		if(style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		{
			addWindowClass (id, "popupSelector");
			enablePopover (id);
		}

		if(_activate)
			activate ();
	}
	else if(!state && isConfigured ())
	{
		enableInput (false);

		if(style.isCustomStyle (Styles::kWindowBehaviorSizable))
			enableWindowResize (id, false);

		removeWindowElement (id);
		safe_release (renderTarget);

		isConfigured (false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::enableInput (bool state, bool capture)
{
	MutableCString frameId;
	getFrameId (frameId);
	MutableCString contentId;
	getContentId (contentId);
	if(state)
	{
		emscripten_set_mousemove_callback (frameId, this, capture, onMouseMoveEvent);
		emscripten_set_mousedown_callback (frameId, this, capture, onMouseDownEvent);
		emscripten_set_mouseup_callback (frameId, this, capture, onMouseUpEvent);
		emscripten_set_click_callback (frameId, this, capture, onClickEvent);
		emscripten_set_dblclick_callback (frameId, this, capture, onDoubleClickEvent);
		emscripten_set_focusin_callback (frameId, this, capture, onFocusInEvent);
		emscripten_set_focusout_callback (frameId, this, capture, onFocusOutEvent);
		emscripten_set_keydown_callback (frameId, this, capture, onKeyDownEvent);
		emscripten_set_keyup_callback (frameId, this, capture, onKeyUpEvent);

		emscripten_set_mouseenter_callback (contentId, this, capture, onMouseEnterEvent);
		emscripten_set_mouseleave_callback (contentId, this, capture, onMouseLeaveEvent);
		emscripten_set_wheel_callback (contentId, this, capture, onMouseWheelEvent);

		emscripten_set_touchstart_callback (contentId, this, capture, onTouchEvent);
		emscripten_set_touchmove_callback (contentId, this, capture, onTouchEvent);
		emscripten_set_touchend_callback (contentId, this, capture, onTouchEvent);
		emscripten_set_touchcancel_callback (contentId, this, capture, onTouchEvent);
	}
	else
	{
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_MOUSEMOVE, (void*)onMouseMoveEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_MOUSEDOWN, (void*)onMouseDownEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_MOUSEUP, (void*)onMouseUpEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_CLICK, (void*)onClickEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_DBLCLICK, (void*)onDoubleClickEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_FOCUSIN, (void*)onFocusInEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_FOCUSOUT, (void*)onFocusOutEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_KEYDOWN, (void*)onKeyDownEvent);
		emscripten_html5_remove_event_listener (frameId, this, EMSCRIPTEN_EVENT_KEYUP, (void*)onKeyUpEvent);

		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_MOUSEENTER, (void*)onMouseEnterEvent);
		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_MOUSELEAVE, (void*)onMouseLeaveEvent);
		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_WHEEL, (void*)onMouseWheelEvent);

		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_TOUCHSTART, (void*)onTouchEvent);
		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_TOUCHMOVE, (void*)onTouchEvent);
		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_TOUCHEND, (void*)onTouchEvent);
		emscripten_html5_remove_event_listener (contentId, this, EMSCRIPTEN_EVENT_TOUCHCANCEL, (void*)onTouchEvent);
	}

	enableDropHandler (id);
	enableDragHandler (id);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WasmWindow::close ()
{
	if(isInCloseEvent ())
		return true;

	if(onClose ())
	{
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

void CCL_API WasmWindow::invalidate (RectRef rect)
{
	if(isInDestroyEvent () || !isConfigured ())
		return;
	
	if(NativeWindowRenderTarget* target = getRenderTarget ())
		if(IMutableRegion* region = target->getInvalidateRegion ())
			region->addRect (rect);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::redraw ()
{
	Rect r;
	getClientRect (r);
	invalidate (r);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WasmWindow::notify (ISubject* subject, MessageRef msg)
{
	if(msg == "dragTimerElapsed")
	{
		if(!mouseDownEventSent)
		{
			onMouseDown (savedMouseDownEvent);
			mouseDownEventSent = true;
		}
	}
	SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

EventResult WasmWindow::handleEvent (SystemEvent& e)
{
	if(e.eventId == "toggle")
		onToggleEvent (e.arg1 == "open" ? true : false);
	else if(e.eventId == "element-resize")
		onResizeEvent ();
	else if(e.eventId == "dpi-changed")
		onDpiChanged (emscripten_get_device_pixel_ratio ());
	else if(e.eventId == "dragenter-effect")
		onDragEnterEffect (e.arg1);
	else if(e.eventId == "dragenter-mime")
		onDragEnterMimeType (e.arg1, e.arg2);
	else if(e.eventId == "dragenter-data")
		onDragEnterData (e.arg1, e.arg2);
	else if(e.eventId == "dragenter")
		e.result = onDragEnterEvent (e.arg1, e.arg2);
	else if(e.eventId == "dragover")
		e.result = onDragOverEvent (e.arg1, e.arg2);
	else if(e.eventId == "dragleave")
		onDragLeaveEvent ();
	else if(e.eventId == "drop-data")
		onDropData (e.arg1, e.arg2);
	else if(e.eventId == "drop")
		onDropEvent (e.arg1, e.arg2);
	else if(e.eventId == "dragstart")
	{
		e.result = onDragStartEvent (e.arg1, e.arg2);
		e.result.setBoolFormat (true);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onToggleEvent (bool state)
{
	if(state == false)
	{
		WasmWindow* topWindow = WasmWindow::cast (Desktop.getTopWindow (kDialogLayer));
		while(topWindow && topWindow != this)
		{
			topWindow = WasmWindow::cast (unknown_cast<Window> (topWindow->parentWindow));
			if(topWindow == this)
				return;
		}
		close ();
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDragEnterEffect (StringRef effect)
{
	dragEffect = IDragSession::kDropNone;
	if(effect == "move" || effect == "copyMove" || effect == "linkMove" || effect == "all" || effect == "uninitialized")
		dragEffect |= IDragSession::kDropMove;
	if(effect == "link" || effect == "linkMove" || effect == "copyLink" || effect == "all" || effect == "uninitialized")
		dragEffect |= IDragSession::kDropCopyShared;
	if(effect == "copy" || effect == "copyMove" || effect == "copyLink" || effect == "all" || effect == "uninitialized")
		dragEffect |= IDragSession::kDropCopyReal;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDragEnterMimeType (int index, StringRef mimeType)
{
	if(index < dragItems.count ())
		dragItems.at (index).mimeType = MutableCString (mimeType, Text::kUTF8);
	else
	{
		ASSERT (dragItems.count () == index)
		dragItems.add ({mimeType, 0});
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onDragEnter (const DragEvent& event)
{
	if(DragSession::isInternalDragActive ())
	{
		safe_release (dragSession);
		dragSession = return_shared (ccl_cast<WasmDragSession> (DragSession::getActiveSession ()));
		onFocus (FocusEvent::kSetFocus);
	}
	return SuperClass::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDragEnterData (int index, StringRef data)
{
	dragItems.at (index).data = data;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WasmWindow::onDragEnterEvent (int x, int y)
{
	safe_release (dragSession);
	if(DragSession::isInternalDragActive ())
		dragSession = return_shared (ccl_cast<WasmDragSession> (DragSession::getActiveSession ()));
	if(dragSession == nullptr)
		dragSession = NEW WasmDragSession;
	for(const DragItem& item : dragItems)
		dragSession->addData (item.mimeType, item.data);
	
	KeyState keyState;
	GUI.getKeyState (keyState);
	DragEvent dragEvent (*dragSession, DragEvent::kDragEnter, {x, y}, keyState);
	dragEvent.keys.keys |= KeyState::kLButton;

	int result = IDragSession::kDropNone;

	setFocus (id);
	if(onDragEnter (dragEvent))
		result = dragSession->getTotalResult ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int WasmWindow::onDragOverEvent (int x, int y)
{
	if(dragSession == nullptr)
		return IDragSession::kDropNone;

	KeyState keyState;
	GUI.getKeyState (keyState);
	DragEvent dragEvent (*dragSession, DragEvent::kDragOver, {x, y}, keyState);
	
	int result = IDragSession::kDropNone;

	setFocus (id);
	if(onDragOver (dragEvent))
		result = dragSession->getTotalResult ();
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDragLeaveEvent ()
{
	if(dragSession == nullptr)
		return;

	KeyState keyState;
	GUI.getKeyState (keyState);
	DragEvent dragEvent (*dragSession, DragEvent::kDragLeave, Point (), keyState);
	onDragLeave (dragEvent);
	onFocus (FocusEvent::kKillFocus);
	safe_release (dragSession);
	dragItems.removeAll ();
	dragEffect = IDragSession::kDropNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDropData (int index, StringRef data)
{
	if(dragSession == nullptr)
		return;

	dragItems.at (index).data = data;
	dragSession->getItems ().removeAll ();
	for(const DragItem& item : dragItems)
		dragSession->addData (item.mimeType, item.data);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WasmWindow::onDropEvent (int x, int y)
{
	if(dragSession == nullptr)
		return;
	
	KeyState keyState;
	GUI.getKeyState (keyState);
	DragEvent dragEvent (*dragSession, DragEvent::kDrop, {x, y}, keyState);
	onDrop (dragEvent);
	dragSession->onDragFinished (dragEvent);

	DragEvent dragLeaveEvent (*dragSession, DragEvent::kDragLeave, Point (), keyState);
	onDragLeave (dragLeaveEvent);

	safe_release (dragSession);
	dragItems.removeAll ();
	dragEffect = IDragSession::kDropNone;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onDragStartEvent (int x, int y)
{
	if(isMoving ())
		return false;
	if(isResizingWindow ())
		return false;

	MouseEvent event (MouseEvent::kMouseDown, {x, y}, 0, System::GetProfileTime ());
	if(event.where.isNull ())
	{
		GUI.getMousePosition (event.where);
		screenToClient (event.where);
	}
	GUI.getKeyState (event.keys);
	event.keys.keys |= KeyState::kLButton;
	event.dragged = true;
	mouseDownEventSent = true;
	onMouseDown (event);
	return DragSession::isInternalDragActive ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseMove (const MouseEvent& _event)
{
	MouseEvent event (_event);
	event.where.y -= getTitleBarHeight ();
	
	if(WasmWindow* topWindow = WasmWindow::cast (Desktop.getTopWindow (kDialogLayer)))
	{
		if(topWindow != this && topWindow->style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
		{
			MouseEvent e (event);
			clientToScreen (e.where);
			topWindow->screenToClient (e.where);
			if(topWindow->onMouseMove (e))
				return true;
		}
	}

	return SuperClass::onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseDown (const MouseEvent& _event)
{
	MouseEvent event (_event);

	bool dragWindow = false;
	if(style.isCustomStyle (Styles::kWindowAppearanceTitleBar))
	{
		int titleBarHeight = getTitleBarHeight ();
		Rect titleBarRect (0, 0, getWidth (), titleBarHeight);
		if(titleBarRect.pointInside (event.where))
			dragWindow = true;
		event.where.y -= titleBarHeight;
	}
	
	if(!dragWindow)
	{
		while(WasmWindow* topWindow = WasmWindow::cast (Desktop.getTopWindow (kDialogLayer)))
		{
			if(topWindow == this || !topWindow->style.isCustomStyle (Styles::kWindowBehaviorPopupSelector))
				break;
			
			MouseEvent e (event);
			clientToScreen (e.where);
			topWindow->screenToClient (e.where);
			if(topWindow->onMouseDown (e))
				return true;
				
			topWindow->close ();
		}
	}

	if(Desktop.getTopWindow (kDialogLayer) == nullptr)
		setFocus (id);

	stopMoveWindow ();
	
	bool result = SuperClass::onMouseDown (event);
	if(dragWindow)
		moveWindow ();
	else if(event.keys.isSet (KeyState::kRButton))
	{
		WasmWindow* topWindow = WasmWindow::cast (Desktop.getTopWindow (kDialogLayer));
		if(topWindow == nullptr)
			popupContextMenu (event.where, false);
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseUp (const MouseEvent& _event)
{
	MouseEvent event (_event);
	event.where.y -= getTitleBarHeight ();
	
	if(WasmWindow* topWindow = WasmWindow::cast (Desktop.getTopWindow (kDialogLayer)))
	{
		if(topWindow != this && topWindow->style.isCustomStyle (Styles::kWindowBehaviorPopupSelector) && topWindow->mouseCaptured ())
		{
			MouseEvent e (event);
			clientToScreen (e.where);
			topWindow->screenToClient (e.where);
			if(topWindow->onMouseUp (e))
				return true;
		}
	}

	stopMoveWindow ();
	bool result = SuperClass::onMouseUp (event);
	cancelDragSession ();
	isResizingWindow (false);
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::captureMouse (bool state)
{
	mouseCaptured (state);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onFocus (const FocusEvent& event)
{
	if(event.eventType == FocusEvent::kKillFocus)
		stopMoveWindow ();
	return SuperClass::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseDownEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	Point p (mouseEvent->targetX, mouseEvent->targetY);
	MouseEvent event (MouseEvent::kMouseDown, p, 0, mouseEvent->timestamp);
	VKey::fromSystemModifiers (event.keys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);

	if(mouseEvent->button == 0)
		event.keys.keys |= KeyState::kLButton;
	else if(mouseEvent->button == 1)
		event.keys.keys |= KeyState::kMButton;
	else if(mouseEvent->button == 2)
		event.keys.keys |= KeyState::kRButton;

	if(event.keys.isSet (KeyState::kLButton))
	{
		This->savedMouseDownEvent = event;
		This->mouseDownEventSent = false;
		(NEW Message ("dragTimerElapsed"))->post (This, kDragTimeout);
	}
	else
		This->onMouseDown (event);
	return false; // don't swallow, allow browser to process drag events
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseUpEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);

	if(!This->mouseDownEventSent)
	{
		This->onMouseDown (This->savedMouseDownEvent);
		This->mouseDownEventSent = true;
	}

	Point p (mouseEvent->targetX, mouseEvent->targetY);
	MouseEvent event (MouseEvent::kMouseUp, p, 0, mouseEvent->timestamp);
	VKey::fromSystemModifiers (event.keys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);

	if(mouseEvent->button == 0)
		event.keys.keys |= KeyState::kLButton;
	else if(mouseEvent->button == 1)
		event.keys.keys |= KeyState::kMButton;
	else if(mouseEvent->button == 2)
		event.keys.keys |= KeyState::kRButton;

	return This->onMouseUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onClickEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onDoubleClickEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	Point p (mouseEvent->targetX, mouseEvent->targetY);
	MouseEvent event (MouseEvent::kMouseDown, p, 0, mouseEvent->timestamp);
	VKey::fromSystemModifiers (event.keys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);
	event.doubleClicked = 1;
	This->mouseDownEventSent = true;
	return This->onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseMoveEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	Point p = Point (mouseEvent->targetX, mouseEvent->targetY);
	MouseEvent event (MouseEvent::kMouseMove, p, 0, mouseEvent->timestamp);
	VKey::fromSystemModifiers (event.keys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);
	bool handled = This->onMouseMove (event);
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMoveWindowEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	This->onMoveWindow (mouseEvent->clientX, mouseEvent->clientY);
	return false; // don't swallow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseEnterEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	Point p (mouseEvent->targetX, mouseEvent->targetY);
	MouseEvent event (MouseEvent::kMouseEnter, p, 0, mouseEvent->timestamp);
	VKey::fromSystemModifiers (event.keys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);
	return This->onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseLeaveEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	Point p (mouseEvent->targetX, mouseEvent->targetY);
	MouseEvent event (MouseEvent::kMouseLeave, p, 0, mouseEvent->timestamp);
	VKey::fromSystemModifiers (event.keys, mouseEvent->ctrlKey, mouseEvent->shiftKey, mouseEvent->altKey, false, mouseEvent->buttons);
	return This->onMouseLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMouseWheelEvent (int eventType, const EmscriptenWheelEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);

	if(mouseEvent->deltaMode != DOM_DELTA_PIXEL)
		return false; // TODO handle page or line scroll modes?

	Point p (mouseEvent->mouse.targetX, mouseEvent->mouse.targetY);
	bool handled = false;
	int type = 0;
	if(mouseEvent->deltaX != 0.)
	{
		type = mouseEvent->deltaX > 0 ? MouseWheelEvent::kWheelRight : MouseWheelEvent::kWheelLeft;
		MouseWheelEvent event (type, p, 0, mouseEvent->deltaX);
		VKey::fromSystemModifiers (event.keys, mouseEvent->mouse.ctrlKey, mouseEvent->mouse.shiftKey, mouseEvent->mouse.altKey, false, mouseEvent->mouse.buttons);
		event.wheelFlags |= MouseWheelEvent::kContinuous;
		handled |= This->onMouseWheel (event);
	}
	if(mouseEvent->deltaY != 0.)
	{
		type = mouseEvent->deltaY > 0 ? MouseWheelEvent::kWheelDown : MouseWheelEvent::kWheelUp;
		MouseWheelEvent event (type, p, 0, -mouseEvent->deltaY);
		VKey::fromSystemModifiers (event.keys, mouseEvent->mouse.ctrlKey, mouseEvent->mouse.shiftKey, mouseEvent->mouse.altKey, false, mouseEvent->mouse.buttons);
		event.wheelFlags |= MouseWheelEvent::kContinuous;
		handled |= This->onMouseWheel (event);
	}
	return handled;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onFocusInEvent (int eventType, const EmscriptenFocusEvent* focusEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	return This->onFocus (FocusEvent::kSetFocus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onFocusOutEvent (int eventType, const EmscriptenFocusEvent* focusEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	return This->onFocus (FocusEvent::kKillFocus);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onKeyDownEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	KeyEvent event;
	VKey::fromSystemEvent (event, *keyEvent, eventType);
	return This->onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onKeyUpEvent (int eventType, const EmscriptenKeyboardEvent* keyEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	KeyEvent event;
	VKey::fromSystemEvent (event, *keyEvent, eventType);
	return This->onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onPageResized (int eventType, const EmscriptenUiEvent* uiEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	if(This->isConfigured () && This->maximized ())
	{
		Rect size (0, 0, uiEvent->windowInnerWidth, uiEvent->windowInnerHeight);
		This->setFrameSize (size);
	}
	return false; // don't swallow
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onCloseEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	return This->close ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onMaximizeEvent (int eventType, const EmscriptenMouseEvent* mouseEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);
	This->maximize (!This->isMaximized ());
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WasmWindow::onTouchEvent (int eventType, const EmscriptenTouchEvent* touchEvent, void* data)
{
	WasmWindow* This = static_cast<WasmWindow*> (data);

	if(!This->getTouchInputState ().getGestureManager ())
		This->getTouchInputState ().setGestureManager (NEW CustomGestureManager (*This));

	TouchCollection touchCollection;
	int touchEventType = TouchEvent::kMove;
	int numTouches = touchEvent->numTouches;
	int endedCount = 0;
	int beganCount = 0;
	int64 time = int64(touchEvent->timestamp);

	for(int i = 0; i < numTouches; i++)
	{
		const EmscriptenTouchPoint& touch = touchEvent->touches[i];

		TouchID id = touch.identifier;
		Point where (touch.targetX, touch.targetY);

		int type = TouchEvent::kMove;
		if(eventType == EMSCRIPTEN_EVENT_TOUCHSTART)
		{
			type = touch.isChanged ? TouchEvent::kBegin : TouchEvent::kMove;
			if(touch.isChanged)
				beganCount++;
		}
		else if(eventType == EMSCRIPTEN_EVENT_TOUCHMOVE)
		{
			type = TouchEvent::kMove;
		}
		else if(eventType == EMSCRIPTEN_EVENT_TOUCHEND || eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL)
		{
			type = touch.isChanged ? TouchEvent::kEnd : TouchEvent::kMove;
			if(touch.isChanged)
				endedCount++;
		}

		TouchInfo info (type, id, where, time);
		touchCollection.add (info);
	}

	if(touchCollection.isEmpty ())
		return true;

	if(beganCount > 0)
		touchEventType = TouchEvent::kBegin;
	else if(endedCount > 0 && endedCount == numTouches)
		touchEventType = TouchEvent::kEnd;  // All touches ended
	else
		touchEventType = TouchEvent::kMove;

	KeyState keyState;
	GUI.getKeyState (keyState);
	TouchEvent event (touchCollection, touchEventType, keyState, TouchEvent::kTouchInput);
	event.eventTime = time;
	This->getTouchInputState ().processTouches (event);

	return true;
}
