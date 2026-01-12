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
// Filename    : ccl/gui/windows/windowbase.cpp
// Description : Window Base
//
//************************************************************************************************

#define DEBUG_LOG 0

#include "ccl/gui/windows/windowbase.h"
#include "ccl/gui/windows/window.h"

#include "ccl/gui/views/viewaccessibility.h"

using namespace CCL;

//************************************************************************************************
// WindowBase
//************************************************************************************************

DEFINE_CLASS (WindowBase, View)

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowBase::WindowBase (const Rect& size, StyleRef style, StringRef title)
: View (size, style, title),
  activeChild (nullptr),
  active (false)
{
	childs.objectCleanup (true);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowBase::~WindowBase ()
{
	lastFocusView = nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::setHelpIdentifier (StringRef id)
{
	helpIdentifier = id;
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

StringRef WindowBase::getHelpIdentifier () const
{
	if(!helpIdentifier.isEmpty ())
		return helpIdentifier;

	return SuperClass::getHelpIdentifier ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* WindowBase::getAccessibilityProvider ()
{
	if(!accessibilityProvider)
		accessibilityProvider = NEW ViewAccessibilityProvider (*this);
	return accessibilityProvider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowBase* WindowBase::getParentWindow () const
{
	return getParent<WindowBase> ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowBase* WindowBase::getFirstActivatableChild () const
{
	ListForEachObject (childs, WindowBase, child)
		if(child->canActivate ())
			return child;
	EndFor
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::addChild (WindowBase* child)
{
	childs.add (child);

	if(isActive () && !activeChild)
		setActiveChild (child);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::removeChild (WindowBase* child)
{
	bool result = childs.remove (child);

	if(child == activeChild && isAttached ())
		setActiveChild (getFirstActivatableChild ());

	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowBase::attached (View* parent)
{
	SuperClass::attached (parent);

	if(WindowBase* parentWindow = getParentWindow ())
		parentWindow->addChild (this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowBase::removed (View* parent)
{
	if(WindowBase* parentWindow = getParentWindow ())
		parentWindow->removeChild (this);

	SuperClass::removed (parent);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowBase::setLastFocusView (View* view)
{
	lastFocusView = view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::isFocusViewAllowed (View& focusView) const
{
	WindowBase* parentBase = focusView.getParent<WindowBase> ();
	return parentBase && isActiveDescendant (parentBase);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::isActiveDescendant (WindowBase* windowBase) const
{
	return windowBase == this || (activeChild && activeChild->isActiveDescendant (windowBase));
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowBase::setActiveChild (WindowBase* child)
{
	if(child && !child->canActivate ())
		return;

	if(isActive ())
	{
		if(activeChild != child)
		{
			if(activeChild)
				activeChild->onActivate (false);
			if(child)
				child->onActivate (true);
		}
	}
	activeChild = child;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API WindowBase::activate ()
{
	if(!isActive () && canActivate ())
	{
		WindowBase* parentWindow = getParentWindow ();
		if(parentWindow)
		{
			parentWindow->setActiveChild (this);
			parentWindow->activate ();
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::canActivate () const
{
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowBase::isActive () const
{
	return active;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::activateViewTree (View& parentView, bool state)
{
	bool hasWindowbase = false;
	if(View* firstChild = parentView.getFirst ())
	{
		if(firstChild == parentView.getLast ())
		{
			if(ccl_cast<WindowBase> (firstChild))
				hasWindowbase = true;
			else if(activateViewTree (*firstChild, state))
				hasWindowbase = true;
		}
		else
		{
			ForEachViewFast (parentView, v)
				if(ccl_cast<WindowBase> (v))
					hasWindowbase = true;
				else if(activateViewTree (*v, state))
					hasWindowbase = true;
			EndFor

			if(hasWindowbase)
			{
				// pass activation to all sibling views of the window base
				ForEachViewFast (parentView, v)
					if(!ccl_cast<WindowBase> (v))
						v->onActivate (state);
				EndFor
			}
		}
	}
	return hasWindowbase;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void WindowBase::onActivate (bool state)
{
	if(state != active)
	{
		CCL_ADD_INDENT (2)
		CCL_PRINTF ("%sWindowBase:onActivate (%s) %s \"%s\" %s\n", CCL_INDENT, state ? "true" : "false", myClass ().getPersistentName (), MutableCString (getTitle ()).str (), MutableCString (getName ()).str ());
		active = state;
		if(active)
		{
			if(lastFocusView)
			{
				CCL_PRINTF ("%sWindowBase: reactivate focusView: %s\n", CCL_INDENT, ((View*)(IView*)lastFocusView)->myClass ().getPersistentName ());
				lastFocusView->takeFocus ();
			}
			else
			{
				// reset focus view if it's in another window base
				if(Window* window = getWindow ())
					if(View* focusView = window->getFocusView ())
						if(!isFocusViewAllowed (*focusView))
						{
							CCL_PRINTF ("%s  remove focus from WindowBase \"%s\"\n", CCL_INDENT, focusView->getParent<WindowBase> () ? MutableCString (focusView->getParent<WindowBase> ()->getTitle ()).str () : "");
							window->killFocusView ();
						}
			}

			if(activeChild)
				activeChild->onActivate (true);
			else
				setActiveChild (getFirstActivatableChild ());
		}
		else
		{
			if(activeChild)
				activeChild->onActivate (false);
		}

		// if no child windows, pass the activation to all subViews (a deactivation always has to be passed to all subViews)
		if(childs.isEmpty () || !state)
			SuperClass::onActivate (state);
		else if(!getFirstActivatableChild ())
		{
			// only non-activatable child windows: active the subtrees without a WindowBase
			activateViewTree (*this, state);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool WindowBase::onMouseDown (const MouseEvent& event)
{
	WindowBase* parentWindow = getParentWindow ();
	if(parentWindow)
		parentWindow->setActiveChild (this);

	return SuperClass::onMouseDown (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

WindowBase* WindowBase::getDeepestActiveWindow ()
{
	if(activeChild)
		if(WindowBase* deepest = activeChild->getDeepestActiveWindow ())
			return deepest;

	if(isActive ())
		return this;

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API WindowBase::getProperty (Variant& var, MemberID propertyId) const
{
	if(propertyId == "isActive")
	{
		var = isActive ();
		return true;
	}
	return SuperClass::getProperty (var, propertyId);
}
