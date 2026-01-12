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
// Filename    : ccl/gui/controls/variantview.h
// Description : Variant View
//
//************************************************************************************************

#ifndef _ccl_variantview_h
#define _ccl_variantview_h

#include "ccl/gui/controls/control.h"

#include "ccl/base/collections/objectarray.h"

namespace CCL {

class ViewAnimator;

//************************************************************************************************
// VariantView
/** Dynamically selects one of its child elements, either via 
	1) a numeric parameter or 2) a property of the controller.

	Example 1: A numeric parameter selects a view by index

	<Variant name="indexValue" attach="fitsize">
		<Label title="Variant 0"/>    <!-- shown when indexValue is 0 -->
		<Label title="Variant 1"/>    <!-- shown when indexValue is 1 -->
	</Variant>

	Example 2: A property of the controller selects a view by property value.

	<Variant property="indexValue" attach="fitsize">
		<Label title="Variant 0"/>    <!-- shown when indexValue is 0 -->
		<Label title="Variant 1"/>    <!-- shown when indexValue is 1 -->
	</Variant>

	Example 3: A boolean parameter switches a view/on/off

	<Variant name="boolValue" attach="fitsize">
		<Label title="Variant"/>    <!-- shown when boolValue is true -->
	</Variant>
*/
//************************************************************************************************

class VariantView: public Control
{
public:
	DECLARE_CLASS (VariantView, Control)

	VariantView (IUnknown* controller, const Rect& size, IParameter* param, StyleRef style);
	VariantView (IUnknown* controller, const Rect& size, CStringRef propertyId, StyleRef style);
	~VariantView ();

	DECLARE_STYLEDEF (customStyles)

	PROPERTY_VARIABLE (TransitionType, transitionType, TransitionType)

	void onChildsAdded (); ///< must be called for initialization after all childs have been added
	Iterator* getVariants () const { return variants.newIterator (); }

private:
	SharedPtr<IUnknown> controller;
	MutableCString propertyId;
	ObjectArray variants;
	int currentIndex;
	bool suppressTransition;
	class HideViewHandler;
	
	VariantView ();

	bool isPropertyMode ();
	void updateSelectedElement (bool observedChanged);
	void selectElement (int index, bool observedChanged);
	bool hideDuringAnimation (ViewAnimator* animator, View* newView, View* oldView);
	void resetLayerOpacity (View* view);
	TransitionType getTransitionType (int index) const;

	// Control
	bool addView (View* view) override;
	void attached (View* parent) override;
	void removed (View* parent) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void onSize (const Point& delta) override;
	void calcSizeLimits () override;
	void onViewsChanged () override;
	void draw (const UpdateRgn& updateRgn) override;
	void paramChanged () override;
	void CCL_API notify (ISubject* subject, MessageRef msg) override;
	IUnknown* CCL_API getController () const override;
	StringRef getHelpIdentifier () const override;

	// IObject
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

} // namespace CCL

#endif // _ccl_variantview_h
