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
// Filename    : ccl/gui/theme/renderer/valuebarrenderer.h
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_valuebarrenderer_h
#define _ccl_valuebarrenderer_h

#include "ccl/gui/theme/renderer/compositedrenderer.h"
#include "ccl/gui/graphics/imaging/imagecache.h"

namespace CCL {

interface IColorParam;

//************************************************************************************************
// ValueBarRenderer
//************************************************************************************************

class ValueBarRenderer: public CompositedRenderer
{
public:
	ValueBarRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	mutable CachedImage valueBarImage;
	
	Color hiliteColor;
	Color hiliteColorAlphaBlend;
	Color hiliteTransparentColor;
	mutable Color colorParamColor;
	bool useColorize;
	int activeFrameIndex;
	int backFrameIndex;
	Pen centerLinePen;
	bool isScalableImage;
	bool initialized;
	
	void initialize ();
	ColorRef getHiliteColor (IColorParam* colorParam) const;
	Rect getSourceRect (View* view, RectRef dstRect) const;
};

DECLARE_VISUALSTYLE_CLASS (ValueBar)

//************************************************************************************************
// ProgressBarRenderer
//************************************************************************************************

class ProgressBarRenderer: public CompositedRenderer
{	
public:
	ProgressBarRenderer (VisualStyle* visualStyle);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	CachedImage background;
	CachedImage foreground;
	CachedImage indicator;
};

DECLARE_VISUALSTYLE_CLASS (ProgressBar)

} // namespace CCL

#endif // _ccl_valuebarrenderer_h
