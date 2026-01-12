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
// Filename    : ccl/gui/views/dialoggroup.cpp
// Description : Group Box
//
//************************************************************************************************

#include "ccl/gui/views/dialoggroup.h"
#include "ccl/gui/theme/themerenderer.h"

#include "ccl/public/gui/graphics/font.h"
#include "ccl/public/gui/graphics/textformat.h"
#include "ccl/gui/graphics/graphicspath.h"

using namespace CCL;

//////////////////////////////////////////////////////////////////////////////////////////////////

BEGIN_STYLEDEF (DialogGroup::customStyles)
	{"primary", Styles::kDialogGroupAppearancePrimary},
	{"secondary", Styles::kDialogGroupAppearanceSecondary},
END_STYLEDEF

//************************************************************************************************
// DialogGroup
//************************************************************************************************

DEFINE_CLASS (DialogGroup, View)
DEFINE_CLASS_UID (DialogGroup, 0x7235d21a, 0xae3c, 0x4d6d, 0x95, 0xd, 0x8e, 0x2f, 0xc0, 0xf5, 0xde, 0xf4)

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogGroup::DialogGroup (const Rect& size, StyleRef style)
: View (size, style),
  renderer (nullptr)
{}

//////////////////////////////////////////////////////////////////////////////////////////////////

DialogGroup::~DialogGroup ()
{
	if(renderer)
		renderer->release ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogGroup::onSize (const Point& delta)
{
	SuperClass::onSize (delta);
	invalidate ();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogGroup::onColorSchemeChanged (const ColorSchemeEvent& event)
{
	if(!visualStyle || visualStyle->hasReferences (event.scheme))
		safe_release (renderer);

	SuperClass::onColorSchemeChanged (event);
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void DialogGroup::draw (const UpdateRgn& updateRgn)
{
	if(renderer == nullptr)
		renderer = getTheme ().createRenderer (ThemePainter::kDialogGroupRenderer, visualStyle);
	if(renderer)
		renderer->draw (this, updateRgn);

	View::draw (updateRgn);
}
