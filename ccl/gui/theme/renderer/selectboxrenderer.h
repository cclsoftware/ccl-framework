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
// Filename    : ccl/gui/theme/renderer/selectboxrenderer.cpp
// Description : SelectBox Renderer
//
//************************************************************************************************

#ifndef _ccl_selectboxrenderer_h
#define _ccl_selectboxrenderer_h

#include "ccl/gui/theme/renderer/bivrenderer.h"

namespace CCL {

//************************************************************************************************
// SelectBoxRenderer
//************************************************************************************************

class SelectBoxRenderer: public BIVRenderer
{
public:
	DECLARE_CLASS_ABSTRACT (SelectBoxRenderer, BIVRenderer)

	SelectBoxRenderer (VisualStyle* visualStyle, View* owner);

	// BIVRenderer
	void applyVisualStyle (RenderAtomTransaction& transaction) override;
	void updateAtoms () override;
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;

protected:
	float buttonFill;			///< select button size relative to the control, 0 = use the size of the image
	bool hideText;
	bool hideButton;
	bool hideImage;
	bool showValueText;
	Coord fitTextSpace;			///< display space the value text was last collapsed against, -1 when unknown

	#if LEGACY_STYLES_SUPPORTED
	float valueImageFill;		///< value image size relative to the control, 0 = fit it into the value area
	#endif

	bool isOnState () const;
	int getElementStateFrame (const Image& image) const;
	int getBackgroundFrame (const Image& image) const;
	void setButtonAlignment (AlignmentRef alignment);
	bool updateValueImage ();
	void updateValueBrush ();
	void checkFitTextSpace ();
};

DECLARE_TYPELIB_ENTRY (SelectBox)

} // namespace CCL

#endif // _ccl_selectboxrenderer_h
