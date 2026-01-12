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
// Filename    : ccl/gui/controls/controlaccessibility.h
// Description : Control Accessibility
//
//************************************************************************************************

#ifndef _ccl_controlaccessibility_h
#define _ccl_controlaccessibility_h

#include "ccl/gui/views/viewaccessibility.h"

namespace CCL {

class Control;

//************************************************************************************************
// ControlAccessibilityProvider
//************************************************************************************************

class ControlAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ControlAccessibilityProvider, ViewAccessibilityProvider)

	ControlAccessibilityProvider (Control& owner);
	
	bool canEdit () const;

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	int CCL_API getElementState () const override;

protected:
	Control& getControl () const;
};

//************************************************************************************************
// ValueControlAccessibilityProvider
//************************************************************************************************

class ValueControlAccessibilityProvider: public ControlAccessibilityProvider,
										 public IAccessibilityValueProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ValueControlAccessibilityProvider, ControlAccessibilityProvider)

	ValueControlAccessibilityProvider (Control& owner);

	// IAccessibilityValueProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	tbool CCL_API isReadOnly () const override;
	tresult CCL_API getValue (String& value) const override;
	tresult CCL_API setValue (StringRef value) const override;
	tbool CCL_API canIncrement () const override;
	tresult CCL_API increment () const override;
	tresult CCL_API decrement () const override;

	CLASS_INTERFACE (IAccessibilityValueProvider, ControlAccessibilityProvider)
};

} // namespace CCL

#endif // _ccl_controlaccessibility_h
