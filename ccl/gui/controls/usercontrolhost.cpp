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
#include "ccl/base/message.h"
#include "ccl/base/storage/settings.h"
#include "ccl/public/collections/unknownlist.h"

namespace CCL {

//************************************************************************************************
// UserControlHostAccessibilityProvider
//************************************************************************************************

class UserControlHostAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (UserControlHostAccessibilityProvider, ViewAccessibilityProvider)

	UserControlHostAccessibilityProvider (UserControlHost& owner);
	~UserControlHostAccessibilityProvider ();

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;

	CLASS_INTERFACES (ViewAccessibilityProvider)

protected:
	UserControlHost& getHost () const
	{
		return static_cast<UserControlHost&> (view);
	}

	IAccessibilityProvider* getCustomProvider () const
	{
		if(IUserControl* control = getHost ().getUserControl ())
			return control->getCustomAccessibilityProvider ();
		return nullptr;
	}

	void populateChildren ();
	void wrapChild (IAccessibilityProvider& child);
};

//************************************************************************************************
// UserControlChildAccessibilityProvider
/** Wraps an externally-implemented IAccessibilityProvider into an AccessibilityProvider. */
//************************************************************************************************

class UserControlChildAccessibilityProvider: public AccessibilityProvider,
											 public IAccessibilitySelectionProvider,
											 public IAccessibilitySelectionContainerProvider
{
public:
	DECLARE_CLASS_ABSTRACT (UserControlChildAccessibilityProvider, AccessibilityProvider)

	UserControlChildAccessibilityProvider (IAccessibilityProvider& wrapped, UserControlHost& host);

	IAccessibilityProvider& getWrapped () const { return *wrappedProvider; }

	// AccessibilityProvider
	void CCL_API getElementName (String& name) const override;
	int CCL_API getElementState () const override;
	AccessibilityElementRole CCL_API getElementRole () const override;
	tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const override;
	tresult CCL_API setElementFocus () override;
	tresult CCL_API makeVisible (tbool relaxed = false) override;
	View* getView () const override;
	AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const override;
	AccessibilityProvider* findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const override;

	// IAccessibilitySelectionProvider
	tbool CCL_API isSelected () const override;
	tresult CCL_API select (tbool state, int flags) override;
	tresult CCL_API getPosition (int& index, int& total) const override;
	IAccessibilityProvider* CCL_API getSelectionContainerProvider () const override;

	// IAccessibilitySelectionContainerProvider
	tresult CCL_API getSelectionProviders (IUnknownList& selection) const override;
	tbool CCL_API isSelectionRequired () const override;
	tbool CCL_API canSelectMultiple () const override;

	CLASS_INTERFACES (AccessibilityProvider)

protected:
	SharedPtr<IAccessibilityProvider> wrappedProvider;
	UserControlHost& host;
	
	AccessibilityProvider* translate (IAccessibilityProvider* provider) const;
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
	makeAccessibilityProvider (false);

	safe_release (userControl);
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
{
	if(IAccessibilityProvider* customProvider = getCustomProvider ())
		ISubject::addObserver (customProvider, this);

	populateChildren ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControlHostAccessibilityProvider::~UserControlHostAccessibilityProvider ()
{
	if(IAccessibilityProvider* customProvider = getCustomProvider ())
		ISubject::removeObserver (customProvider, this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlHostAccessibilityProvider::queryInterface (UIDRef iid, void** ptr)
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

void CCL_API UserControlHostAccessibilityProvider::notify (ISubject* subject, MessageRef msg)
{
	if(msg == IAccessibilityProvider::kChildProviderAdded)
	{
		if(UnknownPtr<IAccessibilityProvider> child = msg[0].asUnknown ())
			wrapChild (*child);
	}
	else if(msg == IAccessibilityProvider::kChildProviderRemoved)
	{
		if(UnknownPtr<IAccessibilityProvider> child = msg[0].asUnknown ())
		{
			ArrayForEachReverse (getChildren (), UserControlChildAccessibilityProvider, adapter)
				if(&adapter->getWrapped () == static_cast<IAccessibilityProvider*> (child))
					removeChildProvider (adapter);
			EndFor
		}
	}
	else
		SuperClass::notify (subject, msg);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHostAccessibilityProvider::populateChildren ()
{
	IAccessibilityProvider* customProvider = getCustomProvider ();
	if(!customProvider)
		return;

	IAccessibilityProvider* child = customProvider->findElementIProvider (AccessibilityDirection::kFirstChild);
	while(child)
	{
		auto* adapter = NEW UserControlChildAccessibilityProvider (*child, getHost ());
		children.add (adapter);
		adapter->setParentProvider (this);
		child = child->findElementIProvider (AccessibilityDirection::kNextSibling);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void UserControlHostAccessibilityProvider::wrapChild (IAccessibilityProvider& child)
{
	AutoPtr<UserControlChildAccessibilityProvider> adapter = NEW UserControlChildAccessibilityProvider (child, getHost ());
	addChildProvider (adapter);
}

//************************************************************************************************
// UserControlChildAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (UserControlChildAccessibilityProvider, AccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

UserControlChildAccessibilityProvider::UserControlChildAccessibilityProvider (IAccessibilityProvider& wrapped, UserControlHost& host)
: wrappedProvider (&wrapped),
  host (host)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::queryInterface (UIDRef iid, void** ptr)
{
	if(ccl_iid<IAccessibilityActionProvider> ().equals (iid))
		return getWrapped ().queryInterface (iid, ptr);
	if(ccl_iid<IAccessibilitySelectionProvider> ().equals (iid) && UnknownPtr<IAccessibilitySelectionProvider> (&getWrapped ()).isValid ())
		QUERY_INTERFACE (IAccessibilitySelectionProvider)
	if(ccl_iid<IAccessibilitySelectionContainerProvider> ().equals (iid) && UnknownPtr<IAccessibilitySelectionContainerProvider> (&getWrapped ()).isValid ())
		QUERY_INTERFACE (IAccessibilitySelectionContainerProvider)
	return SuperClass::queryInterface (iid, ptr);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API UserControlChildAccessibilityProvider::getElementName (String& name) const
{
	getWrapped ().getElementName (name);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API UserControlChildAccessibilityProvider::getElementState () const
{
	return getWrapped ().getElementState ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API UserControlChildAccessibilityProvider::getElementRole () const
{
	return getWrapped ().getElementRole ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const
{
	return getWrapped ().getElementBounds (bounds, space);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::setElementFocus ()
{
	SharedPtr<UserControlChildAccessibilityProvider> lifeGuard (this);
	return getWrapped ().setElementFocus ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::makeVisible (tbool relaxed)
{
	SharedPtr<UserControlChildAccessibilityProvider> lifeGuard (this);
	return getWrapped ().makeVisible (relaxed);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* UserControlChildAccessibilityProvider::getView () const
{
	return &host;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* UserControlChildAccessibilityProvider::findElementProvider (AccessibilityDirection direction) const
{
	switch(direction)
	{
	case AccessibilityDirection::kParent :
		return getParentProvider ();

	case AccessibilityDirection::kNextSibling :
	case AccessibilityDirection::kPreviousSibling :
		if(AccessibilityProvider* parent = getParentProvider ())
		{
			const ObjectArray& siblings = parent->getChildren ();
			int index = siblings.index (this);
			ASSERT (index != -1)
			index += (direction == AccessibilityDirection::kNextSibling) ? 1 : -1;
			if(siblings.isValidIndex (index))
				return static_cast<AccessibilityProvider*> (siblings.at (index));
		}
		return nullptr;

	case AccessibilityDirection::kFirstChild :
		return children.isEmpty () ? nullptr : static_cast<AccessibilityProvider*> (children.first ());

	case AccessibilityDirection::kLastChild :
		return children.isEmpty () ? nullptr : static_cast<AccessibilityProvider*> (children.last ());
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* UserControlChildAccessibilityProvider::findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const
{
	ASSERT (space == AccessibilityCoordSpace::kScreen)
	if(space != AccessibilityCoordSpace::kScreen)
		return nullptr;

	ReverseIterator iter (children.newIterator ());
	while(auto* provider = ccl_cast<AccessibilityProvider> (iter.next ()))
	{
		Rect rect;
		provider->getElementBounds (rect, space);
		if(!rect.pointInside (pos))
			continue;

		if(AccessibilityProvider* result = provider->findElementProviderAt (pos, space))
			return result;

		if(provider->getElementRole () != AccessibilityElementRole::kGroup)
			return provider;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* UserControlChildAccessibilityProvider::translate (IAccessibilityProvider* provider) const
{
	if(!provider)
		return nullptr;

	AccessibilityProvider* parent = getParentProvider ();

	if(provider == host.getUserControl ()->getCustomAccessibilityProvider ())
		return parent;

	if(parent)
	{
		ArrayForEach (parent->getChildren (), UserControlChildAccessibilityProvider, adapter)
			if(&adapter->getWrapped () == provider)
				return adapter;
		EndFor
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControlChildAccessibilityProvider::isSelected () const
{
	if(UnknownPtr<IAccessibilitySelectionProvider> selection = UnknownPtr<IAccessibilitySelectionProvider> (&getWrapped ()))
		return selection->isSelected ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::select (tbool state, int flags)
{
	SharedPtr<UserControlChildAccessibilityProvider> lifeGuard (this);
	if(UnknownPtr<IAccessibilitySelectionProvider> selection = UnknownPtr<IAccessibilitySelectionProvider> (&getWrapped ()))
		return selection->select (state, flags);
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::getPosition (int& index, int& total) const
{
	if(UnknownPtr<IAccessibilitySelectionProvider> selection = UnknownPtr<IAccessibilitySelectionProvider> (&getWrapped ()))
		return selection->getPosition (index, total);
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

IAccessibilityProvider* CCL_API UserControlChildAccessibilityProvider::getSelectionContainerProvider () const
{
	if(UnknownPtr<IAccessibilitySelectionProvider> selection = UnknownPtr<IAccessibilitySelectionProvider> (&getWrapped ()))
		if(IAccessibilityProvider* container = selection->getSelectionContainerProvider ())
			return translate (container);
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API UserControlChildAccessibilityProvider::getSelectionProviders (IUnknownList& selection) const
{
	if(UnknownPtr<IAccessibilitySelectionContainerProvider> container = UnknownPtr<IAccessibilitySelectionContainerProvider> (&getWrapped ()))
	{
		AutoPtr<UnknownList> wrappedSelectionOwner = NEW UnknownList ();
		UnknownList* wrappedSelection = wrappedSelectionOwner;
		container->getSelectionProviders (*wrappedSelection);
		ForEachUnknown (*wrappedSelection, item)
			if(UnknownPtr<IAccessibilityProvider> itemProvider = item)
				if(AccessibilityProvider* translated = translate (itemProvider))
					selection.add (translated->asUnknown (), true);
		EndFor
		return kResultOk;
	}
	return kResultNotImplemented;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControlChildAccessibilityProvider::isSelectionRequired () const
{
	if(UnknownPtr<IAccessibilitySelectionContainerProvider> container = UnknownPtr<IAccessibilitySelectionContainerProvider> (&getWrapped ()))
		return container->isSelectionRequired ();
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API UserControlChildAccessibilityProvider::canSelectMultiple () const
{
	if(UnknownPtr<IAccessibilitySelectionContainerProvider> container = UnknownPtr<IAccessibilitySelectionContainerProvider> (&getWrapped ()))
		return container->canSelectMultiple ();
	return false;
}
