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
// Filename    : ccl/gui/theme/renderer/sliderrenderer.cpp
// Description : Control Renderer
//
//************************************************************************************************

#ifndef _ccl_sliderrenderer_h
#define _ccl_sliderrenderer_h

#include "ccl/gui/theme/renderer/compositedrenderer.h"
#include "ccl/gui/graphics/imaging/imagecache.h"
#include "ccl/gui/views/graphicsport.h"

namespace CCL {

class Slider;
class RangeSlider;
	
//************************************************************************************************
// SliderRenderer
//************************************************************************************************

class SliderRenderer: public CompositedRenderer
{
public:
	SliderRenderer (VisualStyle* visualStyle);

	bool getHandleRect (const Slider* slider, IImage* handleImage, CCL::Rect& rect, float value);
	bool getTrackRect (const Slider* slider, CCL::Rect& rect);

	void draw (View* view, const UpdateRgn& updateRgn) override;
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	virtual bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;

protected:
	CachedImage backImage;
	CachedImage deepBackImage;
	SharedPtr<IImage> handleImage;
	Pen centerLinePen;
	Rect scaleMargin;
	bool initDone;
	bool scaleHandle;
	int margin;
	int scaleOffset;
	int barGraphMargin;
	int barGraphOffset;
	Color handleColor;
	Color disabledHandleColor;
	int disabledBackIndex;
	int disabledHandleIndex;
	
	static constexpr int kSmallSliderSize = 18;
	virtual void init (const View* view);
	virtual void calcBarGraphRects (Rect& src1, Rect& src2, Slider* slider, bool bipolar = false);
	virtual IImage* createColorizedHandle (IImage* sourceImage, ColorRef color);
	
	void drawTickScale (GraphicsPort& port, Slider* slider);
	void drawBackImage (GraphicsPort& port, Slider* slider, CachedImage& backImage);
	void drawBackcolor (GraphicsPort& port, Slider* slider, Color foreColor);
	void drawHandle (GraphicsPort& port, Slider* slider, IImage* handle, float value);
};

DECLARE_VISUALSTYLE_CLASS (Slider)

//************************************************************************************************
// RangeSliderRenderer
//************************************************************************************************

class RangeSliderRenderer: public SliderRenderer
{
public:
	RangeSliderRenderer (VisualStyle* visualStyle);
	
	void draw (View* view, const UpdateRgn& updateRgn) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;
	
protected:
	SharedPtr<IImage> handleImage2;
	
	void init (const View* view) override;
	void calcBarGraphRects (Rect& src1, Rect& src2, Slider* slider, bool bipolar = false) override;
	IImage* createColorizedHandle (IImage* sourceImage = nullptr, ColorRef color = Colors::kBlack) override;
	
	CachedImage invertedBackImage;
	Color centerColor;
	Color invertedCenterColor;
};
	
DECLARE_VISUALSTYLE_CLASS (RangeSlider)
	
} // namespace CCL

#endif // _ccl_sliderrenderer_h
