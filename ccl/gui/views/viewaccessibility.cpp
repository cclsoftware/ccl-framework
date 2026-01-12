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
// Filename    : ccl/gui/views/viewaccessibility.cpp
// Description : View Accessibility
//
//************************************************************************************************

#include "ccl/gui/views/viewaccessibility.h"

#include "ccl/gui/windows/window.h"
#include "ccl/gui/windows/tooltip.h"

using namespace CCL;

//************************************************************************************************
// ViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ViewAccessibilityProvider, AccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ViewAccessibilityProvider::ViewAccessibilityProvider (View& view)
: view (view)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ViewAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kGroup;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CCL_API ViewAccessibilityProvider::getElementName (String& name) const
{
	name = view.getTitle ();
	if(name.isEmpty ())
	{
		ComposedTooltip tooltip (&view);
		name = tooltip;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ViewAccessibilityProvider::getElementState () const
{
	int state = AccessibilityElementState::kTopLevel;
	if(view.isEnabled ())
		state |= AccessibilityElementState::kEnabled;
	if(view.wantsFocus ())
		state |= AccessibilityElementState::kCanFocus;
	if(view.isFocused ())
		state |= AccessibilityElementState::kHasFocus;
	return state;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ViewAccessibilityProvider::setElementFocus ()
{
	if(!view.wantsFocus ())
		return kResultFalse;

	return view.takeFocus () ? kResultOk : kResultFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ViewAccessibilityProvider::getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const
{
	ASSERT (space == AccessibilityCoordSpace::kScreen)
	if(space != AccessibilityCoordSpace::kScreen)
		return kResultInvalidArgument;

	Point screenOffset;
	view.clientToScreen (screenOffset);

	Rect screenRect;
	view.getVisibleClient (screenRect);
	screenRect.offset (screenOffset);

	bounds = screenRect;
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ViewAccessibilityProvider::findElementProvider (AccessibilityDirection direction) const
{
	AccessibilityProvider* result = nullptr;
	switch(direction)
	{
	case AccessibilityDirection::kParent :
		result = parentProvider;
		break;

	case AccessibilityDirection::kNextSibling :
	case AccessibilityDirection::kPreviousSibling :
		if(parentProvider)
		{
			const ObjectArray& siblings = parentProvider->getChildren ();
			int index = siblings.index (this);
			ASSERT (index != -1)

			if(direction == AccessibilityDirection::kNextSibling)
				index++;
			else
				index--;

			if(siblings.isValidIndex (index))
				result = static_cast<AccessibilityProvider*> (siblings.at (index));
		}
		break;

	case AccessibilityDirection::kFirstChild :
		if(!children.isEmpty ())
			result = static_cast<AccessibilityProvider*> (children.first ());
		break;

	case AccessibilityDirection::kLastChild :
		if(!children.isEmpty ())
			result = static_cast<AccessibilityProvider*> (children.last ());
		break;
	}
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ViewAccessibilityProvider::getFocusElementProvider () const
{
	if(auto* window = ccl_cast<Window> (&view))
		if(View* focusView = window->getFocusView ())
			return focusView->getAccessibilityProvider ();
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityProvider* ViewAccessibilityProvider::findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const
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

		if(AccessibilityManager::instance ().findRelatedProvider (provider, AccessibilityRelation::kProxy) != nullptr)
			return provider;

		AccessibilityProvider* result = provider->findElementProviderAt (pos, space);
		if(result)
			return result;

		AccessibilityElementRole role = provider->getElementRole ();
		if(role != AccessibilityElementRole::kGroup)
			return provider;
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

View* ViewAccessibilityProvider::getView () const
{
	return &view;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ViewAccessibilityProvider::makeVisible (tbool relaxed)
{
	Rect clientRect;
	view.getClientRect (clientRect);
	return view.makeVisible (clientRect, relaxed) ? kResultOk : kResultFailed;
}

//************************************************************************************************
// RootViewAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (RootViewAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

RootViewAccessibilityProvider::RootViewAccessibilityProvider (WindowBase& rootView)
: ViewAccessibilityProvider (rootView)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API RootViewAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kRoot;
}
