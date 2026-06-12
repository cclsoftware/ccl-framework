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
// Filename    : ccl/gui/layout/flexboxlayout.h
// Description : Flexbox layout implementation
//
//************************************************************************************************

#ifndef _ccl_flexboxlayout_h
#define _ccl_flexboxlayout_h

#include "ccl/gui/layout/layoutview.h"
#include "ccl/gui/layout/flexboxshared.h"

namespace CCL {

//************************************************************************************************
// FlexboxLayout
//************************************************************************************************

class FlexboxLayout: public Layout
{
public:
	DECLARE_CLASS_ABSTRACT (FlexboxLayout, Layout)

	// Layout
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	LayoutItem* createItem (View* view = nullptr) override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	FlexContainerData flexData;
};

//************************************************************************************************
// FlexLayoutItem
//************************************************************************************************

class FlexLayoutItem: public LayoutItem
{
public:
	DECLARE_CLASS (FlexLayoutItem, LayoutItem)

	FlexLayoutItem ();
	FlexLayoutItem (View* view);

	void initialize (const DesignSize& designSize);
	void updateSizeLimits ();

	const FlexItemData& getFlexItemData () const;

	// LayoutItem
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;

protected:
	FlexItemData flexItemData;
};

} // namespace CCL

#endif // _ccl_flexboxlayout_h
