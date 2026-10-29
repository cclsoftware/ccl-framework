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
// Description : Slider Renderer
//
//************************************************************************************************

#ifndef _ccl_sliderrenderer_h
#define _ccl_sliderrenderer_h

#include "ccl/gui/theme/renderer/compositedrenderer.h"

#include "ccl/gui/graphics/imaging/imagecache.h"

namespace CCL {

class Slider;
class RangeSlider;
class GraphicsPort;
	
//************************************************************************************************
// SliderRenderer
//************************************************************************************************

class SliderRenderer: public CompositedRenderer
{
public:
	SliderRenderer (VisualStyle* visualStyle, View* owner);

	bool getHandleRect (const Slider* slider, IImage* handleImage, Rect& rect, float value);
	bool getTrackRect (const Slider* slider, Rect& rect);

	// ControlRenderer
	void draw (const UpdateRgn& updateRgn) override;
	int hitTest (PointRef where, Point* clickOffset = nullptr) override;
	virtual bool getPartRect (Rect& rect, int partCode) override;

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

	virtual void init ();
	virtual void calcBarGraphRects (Rect& src1, Rect& src2, Slider* slider, bool bipolar = false);
	virtual IImage* createColorizedHandle (IImage* sourceImage, ColorRef color);
	
	void drawTickScale (GraphicsPort& port, Slider* slider);
	void drawBackImage (GraphicsPort& port, Slider* slider, CachedImage& backImage);
	void drawBackcolor (GraphicsPort& port, Slider* slider, Color foreColor);
	void drawHandle (GraphicsPort& port, Slider* slider, IImage* handle, float value);
};

DECLARE_TYPELIB_ENTRY (Slider)

//************************************************************************************************
// RangeSliderRenderer
//************************************************************************************************

class RangeSliderRenderer: public SliderRenderer
{
public:
	RangeSliderRenderer (VisualStyle* visualStyle, View* owner);
	
	// SliderRenderer
	void draw (const UpdateRgn& updateRgn) override;
	bool getPartRect (Rect& rect, int partCode) override;
	
protected:
	SharedPtr<IImage> handleImage2;
	CachedImage invertedBackImage;
	Color centerColor;
	Color invertedCenterColor;
	
	void init () override;
	void calcBarGraphRects (Rect& src1, Rect& src2, Slider* slider, bool bipolar = false) override;
	IImage* createColorizedHandle (IImage* sourceImage = nullptr, ColorRef color = Colors::kBlack) override;
};
	
DECLARE_TYPELIB_ENTRY (RangeSlider)
	
} // namespace CCL

#endif // _ccl_sliderrenderer_h
