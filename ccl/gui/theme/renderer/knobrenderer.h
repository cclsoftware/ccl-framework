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
// Filename    : ccl/gui/theme/renderer/knobrenderer.cpp
// Description : Knob Renderer
//
//************************************************************************************************

#ifndef _ccl_knobrenderer_h
#define _ccl_knobrenderer_h

#include "ccl/gui/theme/renderer/compositedrenderer.h"

namespace CCL {

//************************************************************************************************
// KnobRenderer
//************************************************************************************************

class KnobRenderer: public CompositedRenderer
{
public:
	KnobRenderer (VisualStyle* visualStyle, View* owner);

	// CompositedRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	bool getPartRect (Rect& rect, int partCode) override;
	
protected:	
	SharedPtr<IImage> image;
	SharedPtr<IImage> overlay;
	SharedPtr<IImage> topOverlay;
	SharedPtr<IImage> disabledOverlay;
	bool useFilmStrip;
	bool useOverlayFilmstrip;
	bool useTopOverlayFilmstrip;
	bool drawCircle;
	float circleIndicatorWidth;
	bool drawIndicator;
	bool drawReverseRangeCircle;
	int margin;
	Color indicatorColor;
	Color circleReverseColor;
    float indicatorWidth;
    int indicatorMargin;
	int externalFrameCount;
	float range;
	float linearScaler;
	float linearOffset;
	float logScaler;
	bool allowstretch;
	Rect padding;

	double transformNormalized (double value);
};

DECLARE_TYPELIB_ENTRY (Knob)

} // namespace CCL

#endif // _ccl_knobrenderer_h
