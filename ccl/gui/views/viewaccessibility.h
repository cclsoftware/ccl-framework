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
// Filename    : ccl/gui/views/viewaccessibility.h
// Description : View Accessibility
//
//************************************************************************************************

#ifndef _ccl_viewaccessibility_h
#define _ccl_viewaccessibility_h

#include "ccl/gui/system/accessibility.h"

namespace CCL {

class View;
class WindowBase;

//************************************************************************************************
// ViewAccessibilityProvider
//************************************************************************************************

class ViewAccessibilityProvider: public AccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (ViewAccessibilityProvider, AccessibilityProvider)

	ViewAccessibilityProvider (View& view);

	// AccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
	void CCL_API getElementName (String& name) const override;
	int CCL_API getElementState () const override;
	tresult CCL_API setElementFocus () override;
	tresult CCL_API getElementBounds (Rect& bounds, AccessibilityCoordSpace space) const override;
	AccessibilityProvider* findElementProvider (AccessibilityDirection direction) const override;
	AccessibilityProvider* findElementProviderAt (PointRef pos, AccessibilityCoordSpace space) const override;
	AccessibilityProvider* getFocusElementProvider () const override;
	View* getView () const override;
	tresult CCL_API makeVisible (tbool relaxed = false) override;

protected:
	View& view;
};

//************************************************************************************************
// RootViewAccessibilityProvider
//************************************************************************************************

class RootViewAccessibilityProvider: public ViewAccessibilityProvider
{
public:
	DECLARE_CLASS_ABSTRACT (RootViewAccessibilityProvider, ViewAccessibilityProvider)

	RootViewAccessibilityProvider (WindowBase& rootView);

	// ViewAccessibilityProvider
	AccessibilityElementRole CCL_API getElementRole () const override;
};

} // namespace CCL

#endif // _ccl_viewaccessibility_h
