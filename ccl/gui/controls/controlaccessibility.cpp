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
// Filename    : ccl/gui/controls/controlaccessibility.cpp
// Description : Control Accessibility
//
//************************************************************************************************

#include "ccl/gui/controls/controlaccessibility.h"
#include "ccl/gui/controls/control.h"

#include "ccl/public/gui/iparameter.h"

using namespace CCL;

//************************************************************************************************
// ControlAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ControlAccessibilityProvider, ViewAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ControlAccessibilityProvider::ControlAccessibilityProvider (Control& owner)
: ViewAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

bool ControlAccessibilityProvider::canEdit () const
{
	IParameter* p = getControl ().getParameter ();
	return p && !p->isReadOnly () && p->isEnabled ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

Control& ControlAccessibilityProvider::getControl () const
{
	return static_cast<Control&> (view);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ControlAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kCustom;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

int CCL_API ControlAccessibilityProvider::getElementState () const
{
	int state = SuperClass::getElementState ();
	IParameter* p = getControl ().getParameter ();
	if(p == nullptr || !p->isEnabled ())
		set_flag (state, AccessibilityElementState::kEnabled, false);
	return state;
}

//************************************************************************************************
// ValueControlAccessibilityProvider
//************************************************************************************************

DEFINE_CLASS_ABSTRACT_HIDDEN (ValueControlAccessibilityProvider, ControlAccessibilityProvider)

//////////////////////////////////////////////////////////////////////////////////////////////////

ValueControlAccessibilityProvider::ValueControlAccessibilityProvider (Control& owner)
: ControlAccessibilityProvider (owner)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

AccessibilityElementRole CCL_API ValueControlAccessibilityProvider::getElementRole () const
{
	return AccessibilityElementRole::kSlider;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ValueControlAccessibilityProvider::isReadOnly () const
{
	return !canEdit ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ValueControlAccessibilityProvider::getValue (String& value) const
{
	if(IParameter* p = getControl ().getParameter ())
		p->toString (value);
	else
		value.empty ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ValueControlAccessibilityProvider::setValue (StringRef value) const
{
	if(!canEdit ())
		return kResultFailed;

	if(IParameter* p = getControl ().getParameter ())
		p->fromString (value, true);
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tbool CCL_API ValueControlAccessibilityProvider::canIncrement () const
{
	if(!canEdit ())
		return false;

	IParameter* p = getControl ().getParameter ();
	if(p && p->canIncrement ())
		return true;
	return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ValueControlAccessibilityProvider::increment () const
{
	if(!canEdit ())
		return kResultFailed;

	if(IParameter* p = getControl ().getParameter ())
		p->increment ();
	return kResultOk;
}

//////////////////////////////////////////////////////////////////////////////////////////////////

tresult CCL_API ValueControlAccessibilityProvider::decrement () const
{
	if(!canEdit ())
		return kResultFailed;
	
	if(IParameter* p = getControl ().getParameter ())
		p->decrement ();
	return kResultOk;
}
