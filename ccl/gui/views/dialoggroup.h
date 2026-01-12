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
// Filename    : ccl/gui/views/dialoggroup.h
// Description : View that draws a dialoggroup
//
//************************************************************************************************

#ifndef _ccl_dialoggroup_h
#define _ccl_dialoggroup_h

#include "ccl/gui/views/view.h"

namespace CCL {

class ThemeRenderer;

//************************************************************************************************
// DialogGroup
//************************************************************************************************

class DialogGroup: public View
{
public:
	DECLARE_CLASS (DialogGroup, View)

	DialogGroup (const Rect& size = Rect (), StyleRef style = 0);
	~DialogGroup ();

	DECLARE_STYLEDEF (customStyles)

	// View
	void onSize (const Point& delta) override;
	void onColorSchemeChanged (const ColorSchemeEvent& event) override;
	void draw (const UpdateRgn& updateRgn) override;

protected:
	ThemeRenderer* renderer;
};

} // namespace CCL

#endif // _ccl_dialoggroup_h
