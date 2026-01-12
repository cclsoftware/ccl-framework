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
// Filename    : ccl/gui/controls/usercontrolhost.cpp
// Description : User Control Host
//
//************************************************************************************************

#include "ccl/gui/controls/usercontrolhost.h"

#include "ccl/gui/views/mousehandler.h"
#include "ccl/gui/views/scrollview.h"
#include "ccl/gui/views/viewaccessibility.h"

#include "ccl/gui/windows/window.h"

#include "ccl/base/kernel.h"
#include "ccl/base/storage/settings.h"

namespace CCL {

//************************************************************************************************
// UserControlHostAccessibilityProvider
//************************************************************************************************

class UserControlHostAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (UserControlHostAccessibilityProvider, ViewAccessibilityProvider)

	UserControlHostAccessibilityProvider (UserControlHost& owner);

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const override;
	AccessibilityProvider* findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const override;

	CLASS_INTERFACES (ViewAccessibilityProvider)

protected:
	UserControlHost& getHost () const
	{
		return static_cast<UserControlHost&> (view);
	}

	IAccessibilityProvider* getCustomProvider () const
	{
		return getHost ().getUserControl ()->getCustomAccessibilityProvider ();
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////
// GetViewInterfaceUpwards
//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* GetViewInterfaceUpwards (UIDRef iid, View* view)
{
	IUnknown* iface = nullptr;

	View* v = view;
	while(v)
	{
		v->queryInterface (iid, (void**)&iface);
		if(iface)
		{
			iface->release ();
			return iface;
		}

		if(UserControlHost* uch = ccl_cast<UserControlHost> (v))
		{
			uch->getUserControl ()->queryInterface (iid, (void**)&iface);
			if(iface)
			{
				iface->release ();
				return iface;
			}
		}

		v = v->getParent ();
	}

	// try window settings for layout state...
	Window::getWindowSettings ().queryInterface (iid, (void**)&iface);
	if(iface)
	{
		iface->release ();
		return iface;
	}

	return nullptr;
}

} // namespace CCL

using namespace CCL;

//************************************************************************************************
// UserControlHost
//************************************************************************************************

DEFINE_CLASS (UserControlHost, View)
DEFINE_CLASS_UID (UserControlHost, 0x6c6a508a, 0x4629, 0x4dce, 0xb4, 0xb4, 0x13, 0x98, 0xdf, 0xb0, 0xd6, 0x12)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControlHost::UserControlHost ()
: userControl (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControlHost::~UserControlHost ()
{
	if(userControl)
		userControl->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserControlHost::setUserControl (IUserControl* control)
{
	take_shared (userControl, control);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUserControl* CCL_API UserControlHost::getUserControl ()
{
	return userControl;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef UserControlHost::getHelpIdentifier () const
{
	if(!helpId.isEmpty ())
		return helpId;

	return SuperClass::getHelpIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::setHelpIdentifier (StringRef id)
{
	helpId = id;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserControlHost::setMouseHandler (IMouseHandler* handler)
{
	MouseHandlerDelegate* mouseHandler = handler ? NEW MouseHandlerDelegate (this, handler) : nullptr;
	getWindow ()->setMouseHandler (mouseHandler);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onViewsChanged ()
{
	if(userControl)
		userControl->onViewEvent (ViewEvent (ViewEvent::kViewsChanged));

	View::onViewsChanged ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::attached (View* parent)
{
	View::attached (parent);

	if(userControl)
		userControl->onViewEvent (ViewParentEvent (parent, ViewEvent::kAttached));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::removed (View* parent)
{
	if(userControl)
		userControl->onViewEvent (ViewParentEvent (parent, ViewEvent::kRemoved));

	View::removed (parent); // do it afterwards to avoid doubled removed() calls if user control removes child views
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onActivate (bool state)
{
	View::onActivate (state);

	if(userControl)
		userControl->onViewEvent (ViewEvent (state ? ViewEvent::kActivate : ViewEvent::kDeactivate));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onSize (const Point& delta)
{
	// Note: UserControl delegates event to View::onSize by default!
	if(userControl)
		userControl->onViewEvent (ViewSizeEvent (delta, ViewEvent::kSized));
	else
		View::onSize (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onMove (const Point& delta)
{
	if(userControl)
		userControl->onViewEvent (ViewSizeEvent (delta, ViewEvent::kMoved));

	View::onMove (delta);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onDisplayPropertiesChanged (const DisplayChangedEvent& event)
{
	if(userControl)
		userControl->onViewEvent (event);

	View::invalidate ();
	View::onDisplayPropertiesChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(userControl)
		userControl->onViewEvent (event);

	View::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onVisualStyleChanged ()
{
	SuperClass::onVisualStyleChanged ();

	if(userControl && parent) // (avoid unnecessary event during construction)
		userControl->onViewEvent (ViewEvent (ViewEvent::kVisualStyleChanged));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::onChildSized (View* child, const Point& delta)
{
	SuperClass::onChildSized (child, delta);

	if(userControl)
		userControl->onViewEvent (ViewSizeEvent (delta, ViewEvent::kChildSized, child));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserControlHost::setSizeLimits (const SizeLimit& sizeLimits)
{
	if(sizeLimits.isValid ())
		SuperClass::setSizeLimits (sizeLimits);
	else
	{
		if((privateFlags & (kSizeLimitsValid|kExplicitSizeLimits)) != 0)
			resetSizeLimits ();
		#if 0
		else
			return; // skip notification if nothing changed
		#endif
	}

	// might be called from the user control; pass a notification upwards
	if(parent)
		parent->onChildLimitsChanged (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IUnknown* CCL_API UserControlHost::getController () const
{
	if(userControl)
		return userControl->getController ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControlHost::makeVisible (RectRef rect, tbool relaxed)
{
	UnknownPtr<IScrollable> scrollable (userControl);
	if(scrollable)
		return scrollable->makeVisible (rect, relaxed);
	return View::makeVisible (rect, relaxed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHost::draw (const UpdateRgn& updateRgn)
{
	if(userControl)
	{
		// Note: UserControl delegates event to View::draw by default!
		GraphicsPort port (this);
		userControl->onViewEvent (DrawEvent (port, updateRgn));
	}
	else
		View::draw (updateRgn);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControlHost::canDrawControlBackground () const
{
	if(UnknownPtr<IBackgroundView> bgView = userControl)
		return bgView->canDrawControlBackground ();
	else
		return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserControlHost::drawControlBackground (IGraphics& graphics, RectRef r, PointRef offset)
{
	if(UnknownPtr<IBackgroundView> bgView = userControl)
		bgView->drawControlBackground (graphics, r, offset);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onMouseDown (const MouseEvent& event)
{
	// Note: UserControl delegates event to View::onMouseDown by default!
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onMouseUp (const MouseEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onMouseUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onMouseWheel (const MouseWheelEvent& event)
{
	// Note: UserControl delegates event to View::onMouseWheel by default!
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onMouseWheel (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onMouseEnter (const MouseEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onMouseEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onMouseMove (const MouseEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onMouseMove (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onMouseLeave (const MouseEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onMouseLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

MouseHandler* UserControlHost::createMouseHandler (const MouseEvent& event)
{
	if(userControl)
	{
		IMouseHandler* handler = userControl->createMouseHandler (event);
		if(handler)
			return NEW MouseHandlerDelegate (this, handler);
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

ITouchHandler* UserControlHost::createTouchHandler (const TouchEvent& event)
{
	return userControl ? userControl->createTouchHandler (event) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onContextMenu (const ContextMenuEvent& event)
{
	return userControl && userControl->onViewEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onTrackTooltip (const TooltipEvent& event)
{
	return userControl && userControl->onViewEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onGesture (const GestureEvent& event)
{
	return userControl && userControl->onViewEvent (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onFocus (const FocusEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onFocus (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onKeyDown (const KeyEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onKeyDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onKeyUp (const KeyEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onKeyUp (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IDragHandler* UserControlHost::createDragHandler (const DragEvent& event)
{
	return userControl ? userControl->createDragHandler (event) : nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onDragEnter (const DragEvent& event)
{
	if(userControl && userControl->onViewEvent (event))
		return true;

	return View::onDragEnter (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onDragOver (const DragEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onDragOver (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onDragLeave (const DragEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onDragLeave (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool UserControlHost::onDrop (const DragEvent& event)
{
	if(userControl)
		return userControl->onViewEvent (event) != 0;
	return View::onDrop (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* UserControlHost::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW UserControlHostAccessibilityProvider (*this);
	return accessibilityProvider;
}

//************************************************************************************************
// UserControlHostAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UserControlHostAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControlHostAccessibilityProvider::UserControlHostAccessibilityProvider (UserControlHost& owner)
: ViewAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlHostAccessibilityProvider::queryInterface (CCL::UIDRef iid, void** ptr)
{
	if(SuperClass::queryInterface (iid, ptr) == kResultOk)
		return kResultOk;
	
	if(IAccessibilityProvider* customProvider = getCustomProvider ())
		return customProvider->queryInterface (iid, ptr);

	return kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API UserControlHostAccessibilityProvider::getElementRole () const
{
	if(IAccessibilityProvider* customProvider = getCustomProvider ())
		return customProvider->getElementRole ();
	else
		return SuperClass::getElementRole ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* UserControlHostAccessibilityProvider::findElementProvider (AccessibilityDirection direction) const
{
	AccessibilityProvider* result = nullptr;
	
	IAccessibilityProvider* customProvider = getCustomProvider ();
	if(customProvider)
	{
		switch(direction)
		{
		case AccessibilityDirection::kFirstChild : CCL_FALLTHROUGH
		case AccessibilityDirection::kLastChild :
			result = unknown_cast<AccessibilityProvider> (customProvider->findElementIProvider (direction));
		}
	}

	if(result == nullptr)
		result = SuperClass::findElementProvider (direction);

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* UserControlHostAccessibilityProvider::findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const
{
	AccessibilityProvider* result = nullptr;
	
	IAccessibilityProvider* customProvider = getCustomProvider ();
	if(customProvider)
		result = unknown_cast<AccessibilityProvider> (customProvider->findElementIProviderAt (pos, space));
	
	if(result == nullptr)
		result = SuperClass::findElementProviderAt (pos, space);

	return result;
}
