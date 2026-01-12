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
// Filename    : ccl/gui/theme/renderer/scrollpickerrenderer.h
// Description : Scroll Picker Renderer
//
//************************************************************************************************

#ifndef _ccl_scrollpickerrenderer_h
#define _ccl_scrollpickerrenderer_h

#include "ccl/gui/theme/themerenderer.h"

#include "ccl/gui/graphics/imaging/bitmap.h"
#include "ccl/public/gui/framework/ipalette.h"
#include "ccl/gui/views/graphicsport.h"

namespace CCL {

class ScrollPicker;
class GraphicsDevice;
interface IParameter;

//************************************************************************************************
// ScrollPickerRenderer
//************************************************************************************************

class ScrollPickerRenderer: public ThemeRenderer
{
public:
	ScrollPickerRenderer (VisualStyle* visualStyle);

	void init (ScrollPicker* scrollPicker);

	// ThemeRenderer
	int hitTest (View* view, const Point& loc, Point* clickOffset) override;
	bool getPartRect (const View* view, int partCode, CCL::Rect& rect) override;
	void draw (View* view, const UpdateRgn& updateRgn) override;
	
protected:
	bool wrapAround;
	bool carousel;
	bool vertical;
	bool flatBarrel;
	bool initDone;

	Coord centerSize;
	Coord centerOffset;
	Coord gradientThickness;
	Coord centerCharWidth;
	Coord barrelCharWidth;
	
	Coord itemSize;
	mutable bool multiDigitsMode;
	float visibleItemsFlat;
	int numberOfValues;
	int numberOfRenderValues;
	int overScrollMargin;
	float currentScaleFactor;
	Rect scrollPickerSize;
	Rect firstGradientRect;
	Rect lastGradientRect;
	
	AutoPtr<Bitmap> centerImage;
	AutoPtr<Bitmap> barrelImage;
	
	Font centerTextFont;
	Font barrelTextFont;
	Alignment textAlignment;
	Color centerTextColor;
	Color barrelTextColor;
	Color backcolor;
	Color centerBackcolor;
	Color disabledColor;
	Coord imageMargin;

	GradientBrush firstGradient;
	GradientBrush lastGradient;
	SharedPtr<IImage> centerOverlayImage;
	Rect padding;
	
	enum ViewPart
	{
		kCenter,
		kBarrel
	};

	void drawDigitCenterView (GraphicsPort& port, ScrollPicker* picker);
	void drawDigitBarrelView (GraphicsPort& port, ScrollPicker* picker);
	template<int direction>
	void drawValueDigitsSlice (GraphicsPort& port, ScrollPicker* picker, int currentValue, Coord sourceFrameStartPosition, Coord sourceFrameEndPosition);
	
	void drawValueDigitsSliceVertical (GraphicsPort& port, int currentValue, IImage* sourceBitmap, Coord sourceFrameStartPosition, Coord sourceFrameEndPosition, RectRef targetRect, ViewPart viewId = kCenter);
	void drawValueDigitsCenterSliceHorizontal (GraphicsPort& port, int currentValue, IImage* sourceBitmap, Coord sourceFrameStartPosition, Coord sourceFrameEndPosition, RectRef targetRect);
	void drawValueBarrelDigitsSliceVertical (GraphicsPort& port, ScrollPicker* picker, int currentValue, Coord targetStartCoord, Coord targetEndCoord);
	void drawValueBarrelDigitsSliceHorizontal (GraphicsPort& port, ScrollPicker* picker, int currentValue, Coord targetStartCoord, Coord targetEndCoord);

	template<int direction>
	void drawFlatBarrel (GraphicsPort& port, int scrollPosition);
	template<int direction>
	void drawBarrel (GraphicsPort& port, int scrollPosition);
	template<int direction>
	void drawCenterView (GraphicsPort& port, int scrollPosition);
	template<int direction>
	void drawCarousel (GraphicsPort& port, int scrollPosition, IPalette* palette);
	
	void drawGradients (GraphicsPort& port);
	
	Coord getBarrelPlaneOffset (int scrollPosition) const;
	Coord getBarrelProjectionOffset (float barrelUnitOffset) const;
	void updateViewDependentValues (ScrollPicker* scrollPicker);
	void updateStyle (ScrollPicker* scrollPicker);
	void prepareGradiants ();
	
	Coord getBlankBarrelItemsOffset () const;
	Coord getBarrelPaddingOffset () const;
	
	void constructBitmapAssets (IParameter* scrollPickerParam);
	Coord getCenterViewSize () const;
	Coord getCenterViewOffset () const;
	float getVisibleBarrelItemsCount () const;
	
	void renderScrollPickerBitmap (GraphicsDevice* device, RectRef bitmapRect, IParameter* scrollParam, ViewPart viewId = kCenter);
	void renderWrapAroundElements (GraphicsDevice* device, RectRef bitmapRect, IParameter* scrollParam, ViewPart viewId = kCenter);
	Variant getValueFromIndex (IParameter* scrollParam, int valueIndex) const;
	void getDigitTitle (String& string, int valueIndex) const;
	bool isMultiDigitsMode (ScrollPicker* scrollPicker) const;
	float getLeadingBlankDigits (int digitCount, ViewPart viewId = kCenter) const;
	
	Rect calcCenterLensViewRect () const;
	Rect calculateCenterBitmapRect () const;
	Rect calculateBarrelBitmapRect () const;
};

} // namespace CCL

#endif // _ccl_scrollpickerrenderer_h
