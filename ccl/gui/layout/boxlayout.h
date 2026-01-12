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
// Filename    : ccl/gui/layout/boxlayout.h
// Description : BoxLayout (hbox, vbox)
//
//************************************************************************************************

#ifndef _ccl_boxlayout_h
#define _ccl_boxlayout_h

#include "ccl/gui/layout/anchorlayout.h"

namespace CCL {

//************************************************************************************************
// BoxLayout
//************************************************************************************************

class BoxLayout: public AnchorLayout
{
public:
	DECLARE_CLASS (BoxLayout, AnchorLayout)
	DECLARE_STYLEDEF (customStyles)

	BoxLayout ();

	// Layout
	const StyleDef* getCustomStyles () const override;
	LayoutAlgorithm* createAlgorithm (LayoutContext* context) override;
};

} // namespace CCL

#endif // _ccl_boxlayout_h
