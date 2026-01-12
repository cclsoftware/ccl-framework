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
// Filename    : ccl/gui/layout/tablelayout.h
// Description : TableLayout
//
//************************************************************************************************

#ifndef _ccl_tablelayout_h
#define _ccl_tablelayout_h

#include "ccl/gui/layout/anchorlayout.h"

namespace CCL {

//************************************************************************************************
// TableLayout
//************************************************************************************************

class TableLayout: public AnchorLayout
{
public:
	DECLARE_CLASS (TableLayout, AnchorLayout)

	TableLayout ();

	PROPERTY_VARIABLE (int, numRows, NumRows)
	PROPERTY_VARIABLE (int, numCols, NumColumns)
	PROPERTY_VARIABLE (float, cellRatio, CellRatio)
	PROPERTY_VARIABLE (float, minCellRatio, MinCellRatio)

	// Layout
	bool setAttributes (const SkinAttributes& a) override;
	bool getAttributes (SkinAttributes& a) const override;
	LayoutAlgorithm* createAlgorithm (LayoutContext* context) override;
	tbool CCL_API getProperty (Variant& var, MemberID propertyId) const override;
	tbool CCL_API setProperty (MemberID propertyId, const Variant& var) override;
};

} // namespace CCL

#endif // _ccl_tablelayout_h
